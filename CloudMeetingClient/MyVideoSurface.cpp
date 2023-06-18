#include "myvideosurface.h"
#include <qvideosurfaceformat.h>
#include <qdebug.h>
MyVideoSurface::MyVideoSurface(QObject* parent) :QAbstractVideoSurface(parent)
{

}

QList<QVideoFrame::PixelFormat> MyVideoSurface::supportedPixelFormats(QAbstractVideoBuffer::HandleType handleType) const
{
    if (handleType == QAbstractVideoBuffer::NoHandle)
    {
        return QList<QVideoFrame::PixelFormat>() << QVideoFrame::Format_RGB32
            << QVideoFrame::Format_ARGB32
            << QVideoFrame::Format_ARGB32_Premultiplied
            << QVideoFrame::Format_RGB565
            << QVideoFrame::Format_RGB555;
    }
    else
    {
        return QList<QVideoFrame::PixelFormat>();
    }
}


bool MyVideoSurface::isFormatSupported(const QVideoSurfaceFormat& format) const
{
    // imageFormatFromPixelFormat: ��������Ƶ֡���ظ�ʽ��Ч��ͼ���ʽ
    //pixelFormat: ������Ƶ���е����ظ�ʽ
    return QVideoFrame::imageFormatFromPixelFormat(format.pixelFormat()) != QImage::Format_Invalid;
}

//����Ƶ�������ظ�ʽת���ɸ�ʽ�Եȵ�ͼƬ��ʽ
bool MyVideoSurface::start(const QVideoSurfaceFormat& format)
{
    if (isFormatSupported(format) && !format.frameSize().isEmpty())
    {
        QAbstractVideoSurface::start(format);
        return true;
    }
    return false;
}


//����ÿһ֡��Ƶ��ÿһ֡���ᵽpresent
bool MyVideoSurface::present(const QVideoFrame& frame)
{
    if (!frame.isValid())
    {
        stop();
        return false;
    }
    if (frame.isMapped())
    {
        emit frameAvailable(frame);
    }
    else
    {
        QVideoFrame f(frame);
        f.map(QAbstractVideoBuffer::ReadOnly);
        emit frameAvailable(f);
    }

    return true;
}
