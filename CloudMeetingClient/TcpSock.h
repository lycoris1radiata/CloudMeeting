#pragma once

#include <QThread>
#include <qtcpsocket.h>
#include <qmutex.h>
#include "NetHeader.h"

class TcpSock  : public QThread
{
	Q_OBJECT

public:
	TcpSock(QObject *parent=nullptr);
	~TcpSock();
	bool connectToServer(QString, QString, QIODevice::OpenModeFlag flag= QIODevice::ReadWrite);
	QString errorString();
	void disconnectionFromHost();
	quint32 getLocalIp();
private:
	void run() override;
	QTcpSocket* socktcp;
	QThread* sockThread;
	uchar* sendbuf;
	uchar* recvbuf;
	quint64 hasReceive;
	QMutex m_lock;
	volatile bool isRun;
private slots:
	bool connectServer(QString, QString, QIODevice::OpenModeFlag);
	void sendData(MESG*);
	void closeSocket();
public slots:
	void recvFromSocket();
	void stopImmediately();
	void errorDetect(QAbstractSocket::SocketError error);
signals:
	void socketError(QAbstractSocket::SocketError error);
	void sendTextOver();
	void send_L(MESG*);
};
