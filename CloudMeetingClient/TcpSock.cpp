#include "TcpSock.h"
#include <qhostaddress.h>
#include <qmetaobject.h>
#include <QtEndian>
#include <zlib.h>

extern QUEUE_DATA<MESG> queue_send;
extern QUEUE_DATA<MESG> queue_recv;
extern QUEUE_DATA<MESG> audio_recv;

TcpSock::TcpSock(QObject *parent): QThread(parent)
{
	qRegisterMetaType<QAbstractSocket::SocketError>();
	socktcp = nullptr;
	sockThread = new QThread();
	moveToThread(sockThread);
	connect(sockThread, &QThread::finished, this, &TcpSock::closeSocket);
	sendbuf = (uchar*)malloc(4 * MB);
	recvbuf= (uchar*)malloc(4 * MB);
	hasReceive = 0;
}

TcpSock::~TcpSock()
{
	delete recvbuf;
	delete sendbuf;
	delete socktcp;
	delete sockThread;
}

bool TcpSock::connectToServer(QString ip, QString port, QIODevice::OpenModeFlag flag)
{
	sockThread->start();
	bool retVal;
	QMetaObject::invokeMethod(this, "connectServer", Qt::BlockingQueuedConnection, Q_RETURN_ARG(bool, retVal),
		Q_ARG(QString, ip), Q_ARG(QString, port), Q_ARG(QIODevice::OpenModeFlag, flag));
	if (retVal) {
		start();
		return true;
	}
	else
		return false;
}

QString TcpSock::errorString()
{
	return socktcp->errorString();
}

void TcpSock::disconnectionFromHost()
{
	if (isRunning()) {
		QMutexLocker locker(&m_lock);
		isRun = false;
	}
	if (sockThread->isRunning()) {
		sockThread->quit();
		sockThread->wait();
	}
	queue_recv.clear();
	queue_send.clear();
	audio_recv.clear();
}

quint32 TcpSock::getLocalIp()
{
	if (socktcp->isOpen())
		return socktcp->localAddress().toIPv4Address();
	else
		return -1;
}

void TcpSock::run()
{
	isRun = true;
	for (;;) {
		{
			QMutexLocker locker(&m_lock);
			if (isRun == false)return;
		}
		MESG* send = queue_send.pop_mag();
		if (send == nullptr)continue;
		QMetaObject::invokeMethod(this, "sendData", Q_ARG(MESG*, send));
	}
}

void TcpSock::sendData(MESG* send)
{
	if (socktcp->state() == QAbstractSocket::UnconnectedState) {
		emit sendTextOver();
		if (send->data)free(send->data);
		if (send)free(send);
		return;
	}
	quint64 bytestowrite = 0;
	sendbuf[bytestowrite++] = '$';

	qToBigEndian<quint16>(send->msg_type, sendbuf + bytestowrite);
	bytestowrite += 2;

	quint32 ip = socktcp->localAddress().toIPv4Address();
	qToBigEndian<quint32>(ip, sendbuf + bytestowrite);
	bytestowrite += 4;

	if (send->msg_type == CREATE_MEETING || send->msg_type == AUDIO_SEND ||
		send->msg_type == CLOSE_CAMERA || send->msg_type == IMG_SEND ||
		send->msg_type == TEXT_SEND|| send->msg_type==LOGIN|| send->msg_type==REGISTER) {
		qToBigEndian<quint32>(send->len, sendbuf + bytestowrite);
		bytestowrite += 4;
	}
	else if (send->msg_type == JOIN_MEETING) {
		qToBigEndian<quint32>(send->len, sendbuf + bytestowrite);
		bytestowrite += 4;

		uint32_t room;
		memcpy(&room, send->data, send->len);
		qToBigEndian<quint32>(room, send->data);
	}

	memcpy(sendbuf + bytestowrite, send->data, send->len);
	bytestowrite += send->len;

	sendbuf[bytestowrite++] = '#';

	qint64 hastowrite = bytestowrite;
	qint64 ret = 0, haswrite = 0;
	while ((ret = socktcp->write((char*)sendbuf + haswrite, hastowrite - haswrite)) < hastowrite) {
		if (ret == -1 && socktcp->error() == QAbstractSocket::TemporaryError)
			ret = 0;
		else if (ret == -1) {
			qDebug() << "network error";
			break;
		}
		haswrite += ret;
		hastowrite -= ret;
	}
	socktcp->waitForBytesWritten();
	if (send->msg_type == TEXT_SEND)
		emit sendTextOver();
	if (send->data) {
		free(send->data);
	}
	if (send)
		free(send);
}

void TcpSock::closeSocket()
{
	if (socktcp && socktcp->isOpen()) {
		socktcp->close();
	}
}

void TcpSock::stopImmediately()
{
	{
		QMutexLocker lock(&m_lock);
		if (isRun == true)isRun = false;
	}
	sockThread->quit();
	sockThread->wait();
}

void TcpSock::errorDetect(QAbstractSocket::SocketError error)
{
	qDebug() << "Sock error" << QThread::currentThreadId();
	MESG* msg = (MESG*)malloc(sizeof(MESG));
	if (msg == nullptr) {
		qDebug() << "errdect malloc error";
	}
	else {
		memset(msg, 0, sizeof(MESG));
		if (error == QAbstractSocket::RemoteHostClosedError) {
			msg->msg_type = REMOTEHOSTCLOSE_ERROR;
		}
		else {
			msg->msg_type = OTHERNET_ERROR;
		}
		queue_recv.push_msg(msg);
	}
}

void TcpSock::recvFromSocket() {
	qint64 availbytes = socktcp->bytesAvailable();
	if (availbytes <= 0)
		return;
	qint64 ret = socktcp->read((char*)recvbuf + hasReceive, availbytes);
	if (ret <= 0) {
		qDebug() << "error or no more data";
		return;
	}
	hasReceive+=ret;
	if (hasReceive < MSG_HEADER)
		return;
	else 
	{
		quint32 datasize;
		qFromBigEndian<quint32>(recvbuf + 7, 4, &datasize);
		if ((quint64)datasize + 1 + MSG_HEADER <= hasReceive) 
		{
			if (recvbuf[0] == '$' && recvbuf[MSG_HEADER + datasize] == '#') 
			{
				MSG_TYPE msgtype;
				uint16_t type;
				qFromBigEndian<uint16_t>(recvbuf + 1, 2, &type);
				msgtype = (MSG_TYPE)(type);
				qDebug() << "recv data type:" << msgtype;
				if (msgtype == CREATE_MEETING_RESPONSE || msgtype == JOIN_MEETING_RESPONSE) 
				{
					if (msgtype == CREATE_MEETING_RESPONSE) {
						qint32 roomNo;
						memcpy(&roomNo, recvbuf + MSG_HEADER, datasize);
						MESG* msg = (MESG*)malloc(sizeof(MESG));
						if (msg == nullptr) {
							qDebug() << __LINE__ << "CREATE_MEETING_RESPONSE malloc MESG failed";
						}
						else {
							memset(msg, 0, sizeof(MESG));
							msg->msg_type = msgtype;
							msg->data = (uchar*)malloc((quint64)datasize);
							if (msg->data == nullptr) {
								free(msg->data);
								qDebug() << __LINE__ << "CREATE_MEETING_RESPONSE malloc MESG.data failed";

							}
							else {
								memset(msg->data, 0, (quint64)datasize);
								memcpy(msg->data, &roomNo, datasize);
								msg->len = datasize;
								queue_recv.push_msg(msg);
							}
						}
					}
					else if (msgtype == JOIN_MEETING_RESPONSE) {
						qint32 c;
						memcpy(&c, recvbuf + MSG_HEADER, datasize);
						MESG* msg = (MESG*)malloc(sizeof(MESG));
						if (msg == nullptr) {
							qDebug() << __LINE__ << "JOIN_MEETING_RESPONSE malloc MESG failed";
						}
						else {
							memset(msg, 0, sizeof(MESG));
							msg->msg_type = msgtype;
							msg->data = (uchar*)malloc(datasize);
							if (msg->data == nullptr) {
								free(msg);
								qDebug() << __LINE__ << "JOIN_MEETING_RESPONSE malloc MESG.data failed";

							}
							else {
								memset(msg->data, 0, datasize);
								memcpy(msg->data, &c, datasize);
								msg->len = datasize;
								queue_recv.push_msg(msg);
							}
						}
					}
				}
				else if (msgtype == IMG_RECV ||
					msgtype == PARTNER_JOIN ||
					msgtype == PARTNER_EXIT ||
					msgtype == AUDIO_RECV ||
					msgtype == CLOSE_CAMERA ||
					msgtype == TEXT_RECV) 
				{
					quint32 ip;
					qFromBigEndian<quint32>(recvbuf + 3, 4, &ip);
					if (msgtype == IMG_RECV) 
					{
						uchar* dst = (uchar*)malloc(4 * MB);
						if (dst == nullptr) {
							free(dst);
							qDebug() << __LINE__ << "malloc failed";
						}
						memset(dst, 0, 4 * MB);
						ulong dstlen = 4 * MB;
						QByteArray cc((char*)recvbuf + MSG_HEADER, datasize);
						QByteArray rc = QByteArray::fromBase64(cc);
						int flag = uncompress(dst, &dstlen, (uchar*)rc.data(), rc.size());
						if (flag == 0) {
							MESG* msg = (MESG*)malloc(sizeof(MESG));
							if (msg == nullptr) {
								qDebug() << __LINE__ << "malloc failed";

							}
							else {
								memset(msg, 0, sizeof(MESG));
								msg->msg_type = msgtype;
								msg->data = dst;
								if (msg->data == nullptr) {
									free(msg->data);
									qDebug() << __LINE__ << "malloc failed";

								}
								else {
									msg->len = dstlen;
									msg->ip = ip;
									queue_recv.push_msg(msg);
								}
							}
						}
						else {
							qDebug() << __FILE__ << __LINE__ << "uncompress error:" << flag;
						}
					}
					else if (msgtype == PARTNER_JOIN || msgtype == PARTNER_EXIT || msgtype == CLOSE_CAMERA) 
					{
						MESG* msg = (MESG*)malloc(sizeof(MESG));
						if (msg == nullptr) {
							qDebug() << __LINE__ << "malloc failed";

						}
						else {
							memset(msg, 0, sizeof(MESG));
							msg->msg_type = msgtype;
							msg->ip = ip;
							queue_recv.push_msg(msg);
						}
					}
					else if (msgtype == AUDIO_RECV) 
					{
						uchar* dst = (uchar*)malloc(4 * MB);
						if (dst == nullptr) {
							free(dst);
							qDebug() << __LINE__ << "malloc failed";
						}
						memset(dst, 0, 4 * MB);
						ulong dstlen = 4 * MB;
						QByteArray cc((char*)recvbuf + MSG_HEADER, datasize);
						int flag = uncompress(dst, &dstlen, (uchar*)QByteArray::fromBase64(cc).data(), datasize);
						if (flag == 0) {
							MESG* msg = (MESG*)malloc(sizeof(MESG));
							if (msg == nullptr) {
								qDebug() << __LINE__ << "malloc failed";

							}
							else {
								memset(msg, 0, sizeof(MESG));
								msg->msg_type = msgtype;
								msg->ip = ip;
								msg->data = dst;
								msg->len = dstlen;
								queue_recv.push_msg(msg);
							}
						}
						else {
							qDebug() << __FILE__ << __LINE__ << "uncompress error:" << flag;
						}
					}
					else if (msgtype == TEXT_RECV) 
					{
						uchar* dst = (uchar*)malloc(4 * MB);
						if (dst == nullptr) {
							free(dst);
							qDebug() << __LINE__ << "malloc failed";
						}
						memset(dst, 0, 4 * MB);
						ulong dstlen = 4 * MB;
						int flag = uncompress(dst, &dstlen, recvbuf + MSG_HEADER, datasize);
						if (flag == 0) {
							MESG* msg = (MESG*)malloc(sizeof(MESG));
							if (msg == nullptr) {
								qDebug() << __LINE__ << "malloc failed";

							}
							else {
								memset(msg, 0, sizeof(MESG));
								msg->msg_type = msgtype;
								msg->ip = ip;
								msg->data = dst;
								msg->len = dstlen;
								queue_recv.push_msg(msg);
							}
						}
						else {
							qDebug() << __FILE__ << __LINE__ << "uncompress error:" << flag;
						}
					}
				}
				else if (msgtype == LOGIN_RESPONSE || msgtype == REGISTER_RESPONSE) 
				{
					MESG* msg = (MESG*)malloc(sizeof(MESG));
					if (msg == nullptr) {
						qDebug() << __LINE__ << "malloc failed";

					}
					else {
						memset(msg, 0, sizeof(MESG));
						if (datasize > 0) {
							msg->data=(uchar*)malloc(datasize);
							if (msg->data == nullptr) {
								free(msg);
								qDebug() << __LINE__ << "LOGIN_RESPONSE malloc MESG.data failed";
							}
							else {
								memset(msg->data, 0, datasize);
								memcpy(msg->data, recvbuf + MSG_HEADER, datasize);
								msg->msg_type = msgtype;
								msg->len = datasize;
								emit send_L(msg);
							}
						}
					}
				}
				else {
					qDebug() << "msgtype error";
				}
			}
			else
				qDebug() << "package error";
			memmove_s(recvbuf, 4 * MB, recvbuf + MSG_HEADER + datasize + 1, hasReceive - ((quint64)datasize + 1 + MSG_HEADER));
			hasReceive -= ((quint64)datasize + 1 + MSG_HEADER);
		}
		else
			return;
	}
}

bool TcpSock::connectServer(QString ip, QString port, QIODevice::OpenModeFlag flag) {
	if (socktcp == nullptr)socktcp = new QTcpSocket();
	socktcp->connectToHost(ip, port.toUShort(), flag);
	connect(socktcp, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(errorDetect(QAbstractSocket::SocketError)),Qt::UniqueConnection);
	connect(socktcp, &QTcpSocket::readyRead, this, &TcpSock::recvFromSocket, Qt::UniqueConnection);
	if (socktcp->waitForConnected(5000)) {
		return true;
	}
	socktcp->close();
	return false;
}