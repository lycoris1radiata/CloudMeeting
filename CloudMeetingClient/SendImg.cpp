#include "sendimg.h"
#include "netheader.h"
#include <qdebug.h>
#include <qbuffer.h>
#include <zlib.h>

extern QUEUE_DATA<MESG> queue_send;

SendImg::SendImg(QObject* par) :QThread(par)
{

}


void SendImg::run()
{
    m_isCanRun = true;
    for (;;)
    {
        queue_lock.lock();

        while (imgqueue.size() == 0)
        {
            bool f = queue_waitCond.wait(&queue_lock, WAITSECONDS * 1000);
            if (f == false) 
            {
                QMutexLocker locker(&m_lock);
                if (m_isCanRun == false)
                {
                    queue_lock.unlock();
                    return;
                }
            }
        }

        QByteArray img = imgqueue.front();
        imgqueue.pop_front();
        queue_lock.unlock();
        queue_waitCond.wakeOne(); 

        MESG* imgsend = (MESG*)malloc(sizeof(MESG));
        if (imgsend == NULL)
        {
            qDebug() << "malloc imgsend fail";
        }
        else
        {
            memset(imgsend, 0, sizeof(MESG));
            imgsend->msg_type = IMG_SEND;
            imgsend->len = img.size();
            qDebug() << "img size :" << img.size();
            imgsend->data = (uchar*)malloc(imgsend->len);
            if (imgsend->data == nullptr)
            {
                free(imgsend);
                qDebug() << "send img error";
                continue;
            }
            else
            {
                memset(imgsend->data, 0, imgsend->len);
                memcpy_s(imgsend->data, imgsend->len, img.data(), img.size());
                queue_send.push_msg(imgsend);
            }
        }
    }
}


void SendImg::pushToQueue(QImage img)
{
    QByteArray byte;
    QBuffer buf(&byte);
    buf.open(QIODevice::WriteOnly);
    img.save(&buf, "JPEG");

    uchar* dst=(uchar*)malloc(byte.size() + 10);
    if (dst == nullptr) {
        qDebug() << __FILE__ << __LINE__ << "malloc error:" ;
        return;
    }
    memset(dst, 0, byte.size() + 10);
    ulong dstlen = byte.size() + 10;
    int flag = compress(dst, &dstlen, (Bytef*)byte.data(), byte.size());
    if (flag != 0) {
        qDebug() << __FILE__ << __LINE__ << "compress error:" << flag;
        free(dst);
        return;
    }

    QByteArray ss((char*)dst, dstlen);
    QByteArray vv = ss.toBase64();

    queue_lock.lock();
    while (imgqueue.size() > QUEUE_SIZE)
    {
        queue_waitCond.wait(&queue_lock);
    }
    imgqueue.push_back(vv);
    if(dst)
        free(dst);
    queue_lock.unlock();
    queue_waitCond.wakeOne();
}

void SendImg::ImageCapture(QImage img)
{
    pushToQueue(img);
}

void SendImg::clearImgQueue()
{
    qDebug() << "清空视频队列";
    QMutexLocker locker(&queue_lock);
    imgqueue.clear();
}


void SendImg::stopImmediately()
{
    QMutexLocker locker(&m_lock);
    m_isCanRun = false;
}
