#pragma once

#include <QThread>
#include <qmutex.h>
#include <qqueue.h>
#include "NetHeader.h"

struct M {
	QString str;
	MSG_TYPE type;
	M(QString s,MSG_TYPE t):str(s),type(t){}
};

class SendText  : public QThread
{
	Q_OBJECT

public:
	SendText(QObject *parent=nullptr);
	~SendText();
private:
	QQueue<M> textqueue;
	QMutex queue_lock;
	QWaitCondition queue_cond;
	void run()override;
	QMutex m_lock;
	bool isRun;
public slots:
	void push_Text(MSG_TYPE msgType, QString str = "");
	void stopImmediately();
};
