#include "SendText.h"
#include <qdebug.h>
#include <zlib.h>

extern QUEUE_DATA<MESG> queue_send;

SendText::SendText(QObject *parent)
	: QThread(parent)
{}

SendText::~SendText()
{}

void SendText::run()
{
	isRun = true;
	for (;;) {
		queue_lock.lock();
		while (textqueue.size() == 0) {
			bool f = queue_cond.wait(&queue_lock, WAITSECONDS * 1000);
			if (f == false) {
				QMutexLocker locker(&m_lock);
				if (isRun == false) {
					queue_lock.unlock();
					return;
				}
			}
		}
		M text = textqueue.front();
		textqueue.pop_front();
		queue_lock.unlock();
		queue_cond.wakeOne();
		MESG* send = (MESG*)malloc(sizeof(MESG));
		if (send == nullptr)
		{
			qDebug() << __FILE__ << __LINE__ << "malloc fail";
			continue;
		}
		else
		{
			memset(send, 0, sizeof(MESG));
			if (text.type == CREATE_MEETING || text.type == CLOSE_CAMERA)
			{
				send->len = 0;
				send->data = nullptr;
				send->msg_type = text.type;
				queue_send.push_msg(send);
			}
			else if (text.type == JOIN_MEETING)
			{
				send->msg_type = JOIN_MEETING;
				send->len = 4; 
				send->data = (uchar*)malloc(send->len + 10);
				if (send->data == nullptr)
				{
					qDebug() << __FILE__ << __LINE__ << "malloc fail";
					free(send);
					continue;
				}
				else
				{
					memset(send->data, 0, send->len + 10);
					quint32 roomno = text.str.toUInt();
					memcpy(send->data, &roomno, sizeof(roomno));
					queue_send.push_msg(send);
				}
			}
			else if (text.type == TEXT_SEND)
			{
				send->msg_type = TEXT_SEND;
				send->data =(uchar*)malloc(text.str.size()+ 8);
				if (send->data == nullptr)
				{
					qDebug() << __FILE__ << __LINE__ << "malloc error";
					free(send);
					continue;
				}
				memset(send->data, 0, text.str.size() + 8);
				unsigned long dstlen = text.str.size() + 8;
				int flag=compress(send->data, &dstlen, (Bytef*)text.str.toStdString().data(), text.str.size());
				if (flag == 0) {
					send->len = dstlen;
					queue_send.push_msg(send);
				}
				else {
					qDebug() << __FILE__ << __LINE__ << "compress error:"<<flag;
				}
			}
		}
	}
}

void SendText::stopImmediately()
{
	QMutexLocker locker(&m_lock);
	isRun = false;
}

void SendText::push_Text(MSG_TYPE msgType, QString str) {
	queue_lock.lock();
	while (textqueue.size() > QUEUE_SIZE) {
		queue_cond.wait(&queue_lock);
	}
	textqueue.push_back(M(str, msgType));
	queue_lock.unlock();
	queue_cond.wakeOne();
}