#pragma once

#include "NetHeader.h"
#include <QThread>
#include <qmutex.h>

class RecvDeal  : public QThread
{
	Q_OBJECT

public:
	RecvDeal(QObject *parent=nullptr);
	~RecvDeal();
	void run()override;
private:
	QMutex m_lock;
	bool isRun;
signals:
	void DataRecv(MESG*);
public slots:
	void stopImmediately();
};
