#pragma once

#include <QtWidgets/QWidget>
#include "ui_Widget.h"
#include <qthread.h>
#include "NetHeader.h"
#include "Partner.h"
#include <qmap.h>
#include <qcamera.h>
#include <qcameraimagecapture.h>
#include <qhostaddress.h>
#include <qimage.h>
#include <qboxlayout.h>

QT_BEGIN_NAMESPACE
namespace Ui { class WidgetClass; };
QT_END_NAMESPACE

class TcpSock;
class RecvDeal;
class SendText;
class SendImg;
class AudioInput;
class AudioOutput;
class MyVideoSurface;

class Widget : public QWidget
{
    Q_OBJECT

public:
    Widget(QWidget *parent = nullptr);
    ~Widget();
private:
    Ui::WidgetClass *ui;
    QVBoxLayout* layout;

    TcpSock* _mytcpSocket;

    SendText* sendText_;
    QThread* textThread_;

    SendImg* _sendImg;
    QThread* _imgThread;

    RecvDeal* _recvThread;
    SendText* _sendText;
    QThread* _textThread;

    AudioInput* _ainput;
    QThread* _ainputThread;
    AudioOutput* _aoutput;
    
    QCamera* _camera;
    QCameraImageCapture* _imagecapture;
    MyVideoSurface* _myvideosurface;

    quint32 m_ip;

    bool _createmeet;
    bool _joinmeet;
    bool _openCamera;

    QMap<quint32, Partner*> partner;
    Partner* addPartner(quint32);
    void removePartner(quint32);
    void clearPartner();
    void closeImg(quint32);
    void paintEvent(QPaintEvent* event);

public slots:

    void on_createmeetBtn_clicked();
    void on_exitmeetBtn_clicked();
    void on_openVedio_clicked();
    void on_openAudio_clicked();
    void on_joinmeetBtn_clicked();
    void on_sendmsg_clicked();
    void on_sendimg_clicked();
    

    void dataDeal(MESG* msg);
    void textSend();
    void cameraImageCapture(QVideoFrame frame);
    void cameraError(QCamera::Error);
    void audioError(QString);
    void speaks(QString);
    void recvIp(quint32);

    void recvok_L(QString ip, QString port);
signals:
    void pushText(MSG_TYPE msgtype, QString str="");
    void pushImg(QImage);
    void stopAudio();
    void startAudio();
    void send_quit();
    void send_L(bool net);
};
