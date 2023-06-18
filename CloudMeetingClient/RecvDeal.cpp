#include "RecvDeal.h"
#include <qmetatype.h>
#include <qdebug.h>
#include <qmutex.h>

extern QUEUE_DATA<MESG> queue_recv;

RecvDeal::RecvDeal(QObject *parent)
	: QThread(parent)
{
	qRegisterMetaType<MESG*>();
	isRun = true;
}

RecvDeal::~RecvDeal()
{}

void RecvDeal::run()
{
	for (;;) {
		{
			QMutexLocker locker(&m_lock);
			if (isRun == false) {
				return;
			}
		}
		MESG* msg = queue_recv.pop_mag();
		if (msg == nullptr)continue;
		emit DataRecv(msg);
	}
}

void RecvDeal::stopImmediately() {
	QMutexLocker locker(&m_lock);
	isRun = false;
}