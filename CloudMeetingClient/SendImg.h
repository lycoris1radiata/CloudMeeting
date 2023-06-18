#pragma once
#include <QThread>
#include <qqueue.h>
#include <qmutex.h>
#include <qimage.h>
#include <qvideoframe.h>
#include <qwaitcondition.h>

class SendImg : public QThread
{
    Q_OBJECT
private:
    QQueue<QByteArray> imgqueue;
    QMutex queue_lock;
    QWaitCondition queue_waitCond;
    void run() override;
    QMutex m_lock;
    volatile bool m_isCanRun;
public:
    SendImg(QObject* par = NULL);

    void pushToQueue(QImage);
public slots:
    void ImageCapture(QImage); 
    void clearImgQueue(); 
    void stopImmediately();
};
