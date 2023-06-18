#include "Widget.h"
#include "TcpSock.h"
#include "RecvDeal.h"
#include "SendText.h"
#include "SendImg.h"
#include "AudioInput.h"
#include "AudioOutput.h"
#include "MyVideoSurface.h"
#include <qmessagebox.h>
#include <qhostaddress.h>
#include <qdatetime.h>
#include <qsound.h>
#include <qscrollbar.h>
#include <qfiledialog.h>
#include <qtextcodec.h>

#ifdef _MSC_VER
#pragma execution_character_set("utf-8")
#endif


extern QUEUE_DATA<MESG> queue_send;
extern QUEUE_DATA<MESG> queue_recv;
extern QUEUE_DATA<MESG> audio_recv;

Widget::Widget(QWidget *parent): QWidget(parent),ui(new Ui::WidgetClass())
{
    ui->setupUi(this);
    qRegisterMetaType<MSG_TYPE>();

    layout=new QVBoxLayout(ui->scrollAreaWidgetContents);
    layout->setSpacing(3);
    
    _createmeet = false;
    _openCamera = false;
    _joinmeet = false;

    ui->exitmeetBtn->setDisabled(true);
    ui->joinmeetBtn->setDisabled(true);
    ui->createmeetBtn->setDisabled(true);
    ui->openAudio->setDisabled(true);
    ui->openVedio->setDisabled(true);
    ui->sendmsg->setDisabled(false);
    ui->sendimg->setDisabled(true);

    _sendImg = new SendImg();
    _imgThread = new QThread();
    _sendImg->moveToThread(_imgThread);
    _sendImg->start();

    _mytcpSocket = new TcpSock();
    connect(_mytcpSocket,&TcpSock::sendTextOver,this,&Widget::textSend);

    _sendText = new SendText();
    _textThread = new QThread();
    _sendText->moveToThread(_textThread);
    _textThread->start();
    _sendText->start(); 
    connect(this, SIGNAL(pushText(MSG_TYPE, QString)), _sendText, SLOT(push_Text(MSG_TYPE, QString)));

    _camera = new QCamera(this);
    connect(_camera, SIGNAL(error(QCamera::Error)), this, SLOT(cameraError(QCamera::Error)));
    _imagecapture = new QCameraImageCapture(_camera);
    _myvideosurface = new MyVideoSurface(this);

    connect(_myvideosurface, SIGNAL(frameAvailable(QVideoFrame)), this, SLOT(cameraImageCapture(QVideoFrame)));
    connect(this, SIGNAL(pushImg(QImage)), _sendImg, SLOT(ImageCapture(QImage)));
    connect(_imgThread, SIGNAL(finished()), _sendImg, SLOT(clearImgQueue()));

    _recvThread = new RecvDeal();
    connect(_recvThread, SIGNAL(DataRecv(MESG*)), this, SLOT(dataDeal(MESG*)), Qt::BlockingQueuedConnection);
    _recvThread->start();

    _camera->setViewfinder(_myvideosurface);
    _camera->setCaptureMode(QCamera::CaptureStillImage);

    _ainput = new AudioInput();
    _ainputThread = new QThread();
    _ainput->moveToThread(_ainputThread);

    _aoutput = new AudioOutput();
    _ainputThread->start();
    _aoutput->start(); 

    connect(this, SIGNAL(startAudio()), _ainput, SLOT(startCollect()));
    connect(this, SIGNAL(stopAudio()), _ainput, SLOT(stopCollect()));
    connect(_ainput, SIGNAL(audioinputerror(QString)), this, SLOT(audioError(QString)));
    connect(_aoutput, SIGNAL(audiooutputerror(QString)), this, SLOT(audioError(QString)));
    connect(_aoutput, SIGNAL(speaker(QString)), this, SLOT(speaks(QString)));

    ui->scrollArea->verticalScrollBar()->setStyleSheet("QScrollBar:vertical { width:8px; background:rgba(0,0,0,0%); margin:0px,0px,0px,0px; padding-top:9px; padding-bottom:9px; }QScrollBar::handle:vertical { width:8px; background:rgba(0,0,0,25%); border-radius:4px; min-height:20; }QScrollBar::handle:vertical:hover { width:8px; background:rgba(0,0,0,50%); border-radius:4px; min-height:20; } QScrollBar::add-line:vertical { height:9px;width:8px;subcontrol-position:bottom; } QScrollBar::sub-line:vertical { height:9px;width:8px;subcontrol-position:top; } QScrollBar::add-line:vertical:hover { height:9px;width:8px;  subcontrol-position:bottom; } QScrollBar::sub-line:vertical:hover { height:9px;width:8px; subcontrol-position:top; } QScrollBar::add-page:vertical,QScrollBar::sub-page:vertical { background:rgba(0,0,0,10%); border-radius:4px; }");
    ui->listWidget->setStyleSheet("QScrollBar:vertical { width:8px; background:rgba(0,0,0,0%); margin:0px,0px,0px,0px; padding-top:9px; padding-bottom:9px; } QScrollBar::handle:vertical { width:8px; background:rgba(0,0,0,25%); border-radius:4px; min-height:20; } QScrollBar::handle:vertical:hover { width:8px; background:rgba(0,0,0,50%); border-radius:4px; min-height:20; } QScrollBar::add-line:vertical { height:9px;width:8px;subcontrol-position:bottom; } QScrollBar::sub-line:vertical { height:9px;width:8px; subcontrol-position:top; } QScrollBar::add-line:vertical:hover { height:9px;width:8px;  subcontrol-position:bottom; } QScrollBar::sub-line:vertical:hover { height:9px;width:8px; subcontrol-position:top; } QScrollBar::add-page:vertical,QScrollBar::sub-page:vertical { background:rgba(0,0,0,10%); border-radius:4px; }");

    QFont te_font = this->font();
    te_font.setFamily("MicrosoftYaHei");
    te_font.setPointSize(12);
    ui->listWidget->setFont(te_font);
}

Widget::~Widget()
{
    if (_mytcpSocket->isRunning())
    {
        _mytcpSocket->stopImmediately();
        _mytcpSocket->wait();
    }
    delete _mytcpSocket;

    if (_recvThread->isRunning())
    {
        _recvThread->stopImmediately();
        _recvThread->wait();
    }
    delete _recvThread;
    if (_imgThread->isRunning())
    {
        _imgThread->quit();
        _imgThread->wait();
    }

    if (_sendImg->isRunning())
    {
        _sendImg->stopImmediately();
        _sendImg->wait();
    }
    delete _sendImg;
    delete _imgThread;

    if (_textThread->isRunning())
    {
        _textThread->quit();
        _textThread->wait();
    }

    if (_sendText->isRunning())
    {
        _sendText->stopImmediately();
        _sendText->wait();
    }
    delete _sendText;
    delete _textThread;
    if (_ainputThread->isRunning())
    {
        _ainputThread->quit();
        _ainputThread->wait();
    }
    delete _ainputThread;
    delete _ainput;
    if (_aoutput->isRunning())
    {
        _aoutput->stopImmediately();
        _aoutput->wait();
    }
    delete _aoutput;

    delete ui;
}

void Widget::on_createmeetBtn_clicked()
{
    if (false == _createmeet)
    {
        ui->createmeetBtn->setDisabled(true);
        ui->openAudio->setDisabled(true);
        ui->openVedio->setDisabled(true);
        ui->exitmeetBtn->setDisabled(true);
        emit pushText(CREATE_MEETING);
    }
}

void Widget::on_exitmeetBtn_clicked()
{
    if (_camera->status() == QCamera::ActiveStatus)
    {
        _camera->stop();
    }

    ui->createmeetBtn->setDisabled(false);
    ui->exitmeetBtn->setDisabled(true);
    _createmeet = false;
    _joinmeet = false;

    clearPartner();
    _mytcpSocket->disconnectionFromHost();
    _mytcpSocket->wait();

    ui->outlog->setText(tr("已退出会议"));
    ui->groupBox->setTitle(QString("主屏幕"));
  
    while (ui->listWidget->count() > 0)
    {
        ui->listWidget->clear();
    }

    emit send_quit();
}

void Widget::on_openVedio_clicked()
{
    if (_camera->status() == QCamera::ActiveStatus)
    {
        _camera->stop();
        if (_camera->error() == QCamera::NoError)
        {
            _imgThread->quit();
            _imgThread->wait();
            ui->openVedio->setText("开启摄像头");
            emit pushText(CLOSE_CAMERA);
        }
        closeImg(m_ip);
    }
    else
    {
        _camera->start();
        if (_camera->error() == QCamera::NoError)
        {
            _imgThread->start();
            ui->openVedio->setText("关闭摄像头");
        }
    }
}

void Widget::closeImg(quint32 ip)
{
    if (!partner.contains(ip))
    {
        qDebug() << "close img error";
        return;
    }
    Partner* p = partner[ip];
    p->setpic(QImage(":/image/1"));

    if (m_ip == ip)
    {
        ui->mainshow_label->setPixmap(QPixmap::fromImage(QImage(":/image/1").scaled(ui->mainshow_label->size())));
    }
}

void Widget::paintEvent(QPaintEvent* event)
{
     Q_UNUSED(event);
}

void Widget::on_openAudio_clicked()
{
    if (!_createmeet && !_joinmeet) return;
    if (ui->openAudio->text().toUtf8() == QString(OPENAUDIO).toUtf8())
    {
        emit startAudio();
        ui->openAudio->setText(QString(CLOSEAUDIO).toUtf8());
    }
    else
    {
        emit stopAudio();
        ui->openAudio->setText(QString(OPENAUDIO).toUtf8());
    }
}

void Widget::on_joinmeetBtn_clicked()
{
    QString roomNo = ui->meetno->text();
    emit pushText(JOIN_MEETING, roomNo);
}

void Widget::on_sendmsg_clicked()
{
    QString msg = ui->plainTextEdit->toPlainText().trimmed();
    if (msg.size() == 0)
    {
        qDebug() << "empty";
        return;
    }
    qDebug() << msg;
    ui->plainTextEdit->clear();

    QString msgshow=QHostAddress(_mytcpSocket->getLocalIp()).toString() + ": " + msg;
    if (ui->listWidget->count() == 0) {
        QDateTime time = QDateTime::fromTime_t(QDateTime::currentDateTimeUtc().toTime_t());
        time.setTimeSpec(Qt::UTC);
        QString m_Time = time.toString("yyyy-MM-dd ddd hh:mm:ss");

        QLabel* label = new QLabel;
        label->setText(m_Time);
        label->setAlignment(Qt::AlignCenter);
        QListWidgetItem* item = new QListWidgetItem;
        ui->listWidget->addItem(item);
        ui->listWidget->setItemWidget(item, label);
        ui->listWidget->addItem(msgshow);
    }
    else
        ui->listWidget->addItem(msgshow);

    emit pushText(TEXT_SEND, msg);
}

void Widget::on_sendimg_clicked() {
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open Image"), ".", tr("Image Files(*.png *.jpg *jpeg *.bmp)"));
    QTextCodec* code = QTextCodec::codecForName("GB2312");
    QString imagePath = code->fromUnicode(fileName);
    QImage img;
    img.load(imagePath);
    _imgThread->start();
    if(!img.isNull())
        emit pushImg(img);
}

Partner* Widget::addPartner(quint32 ip)
{
    if(partner.contains(ip))
        return nullptr;
    Partner* p = new Partner(ui->scrollAreaWidgetContents, ip);
    if (p == nullptr) {
        qDebug() << "new Partner error";
        return nullptr;
    }
    else {
        connect(p,&Partner::sendip,this,&Widget::recvIp);
        partner.insert(ip, p);
        layout->addWidget(p, 1);
        if (partner.size() > 1) {
            connect(this, SIGNAL(volumnChange(int)), _ainput, SLOT(setVolumn(int)), Qt::UniqueConnection);
            connect(this, SIGNAL(volumnChange(int)), _aoutput, SLOT(setVolumn(int)), Qt::UniqueConnection);
            ui->openAudio->setDisabled(false);
            _aoutput->startPlay();
        }
        return p;
    }
}

void Widget::removePartner(quint32 ip)
{
    if (partner.contains(ip)) {
        Partner* p = partner[ip];
        disconnect(p, &Partner::sendip, this, &Widget::recvIp);
        layout->removeWidget(p);
        delete p;
        partner.remove(ip);

        if (partner.size() <= 1)
        {
            disconnect(_ainput, SLOT(setVolumn(int)));
            disconnect(_aoutput, SLOT(setVolumn(int)));
            _ainput->stopCollect();
            _aoutput->stopPlay();
            ui->openAudio->setText(QString(OPENAUDIO).toUtf8());
            ui->openAudio->setDisabled(true);
        }
    }
}

void Widget::clearPartner()
{
    ui->mainshow_label->setPixmap(QPixmap());
    if (partner.size() == 0) return;

    QMap<quint32, Partner*>::iterator iter = partner.begin();
    while (iter != partner.end())
    {
        quint32 ip = iter.key();
        iter++;
        Partner* p = partner.take(ip);
        layout->removeWidget(p);
        delete p;
        p = nullptr;
    }

    disconnect(_ainput, SLOT(setVolumn(int)));
    disconnect(_aoutput, SLOT(setVolumn(int)));
    _ainput->stopCollect();
    _aoutput->stopPlay();
    ui->openAudio->setText(QString(CLOSEAUDIO).toUtf8());
    ui->openAudio->setDisabled(true);

    if (_imgThread->isRunning())
    {
        _imgThread->quit();
        _imgThread->wait();
    }
    ui->openVedio->setText(QString(OPENVIDEO).toUtf8());
    ui->openVedio->setDisabled(true);
}

void Widget::dataDeal(MESG* msg)
{
    if (msg->msg_type == CREATE_MEETING_RESPONSE)
    {
        int roomno;
        memcpy(&roomno, msg->data, msg->len);

        if (roomno != 0)
        {
            QMessageBox::information(this, "Room No", QString("房间号：%1").arg(roomno), QMessageBox::Yes, QMessageBox::Yes);

            ui->groupBox->setTitle(QString("主屏幕(房间号: %1)").arg(roomno));
            ui->outlog->setText(QString("创建成功 房间号: %1").arg(roomno));
            _createmeet = true;
            ui->exitmeetBtn->setDisabled(false);
            ui->openVedio->setDisabled(false);
            ui->joinmeetBtn->setDisabled(true);

            addPartner(m_ip);
            ui->groupBox->setTitle(QHostAddress(_mytcpSocket->getLocalIp()).toString());
            ui->mainshow_label->setPixmap(QPixmap::fromImage(QImage(":/image/1").scaled(ui->mainshow_label->size())));
        }
        else
        {
            _createmeet = false;
            QMessageBox::information(this, "Room Information", QString("无可用房间"), QMessageBox::Yes, QMessageBox::Yes);
            ui->outlog->setText(QString("无可用房间"));
            ui->createmeetBtn->setDisabled(false);
        }
    }
    else if (msg->msg_type == JOIN_MEETING_RESPONSE)
    {
        qint32 c;
        memcpy(&c, msg->data, msg->len);
        if (c == 0)
        {
            QMessageBox::information(this, "Meeting Error", tr("会议不存在"), QMessageBox::Yes, QMessageBox::Yes);
            ui->outlog->setText(QString("会议不存在"));
            ui->exitmeetBtn->setDisabled(true);
            ui->openVedio->setDisabled(true);
            ui->joinmeetBtn->setDisabled(false);
            _joinmeet = false;
        }
        else
        {
            QMessageBox::warning(this, "Meeting information", "加入成功", QMessageBox::Yes, QMessageBox::Yes);
            ui->outlog->setText(QString("加入成功"));
            //添加用户自己
            addPartner(m_ip);
            ui->groupBox->setTitle(QHostAddress(_mytcpSocket->getLocalIp()).toString());
            ui->mainshow_label->setPixmap(QPixmap::fromImage(QImage(":/image/1").scaled(ui->mainshow_label->size())));
            ui->joinmeetBtn->setDisabled(true);
            ui->exitmeetBtn->setDisabled(false);
            ui->createmeetBtn->setDisabled(true);
            ui->openVedio->setDisabled(true);
            _joinmeet = true;
        }
    }
    else if (msg->msg_type == IMG_RECV)
    {
        QHostAddress a(msg->ip);
        qDebug() << a.toString();
        QImage img;
        img.loadFromData(msg->data, msg->len);
        if (partner.count(msg->ip) == 1)
        {
            Partner* p = partner[msg->ip];
            p->setpic(img);
        }
        else
        {
            Partner* p = addPartner(msg->ip);
            p->setpic(img);
        }

        if (msg->ip == m_ip)
        {
            ui->mainshow_label->setPixmap(QPixmap::fromImage(img).scaled(ui->mainshow_label->size()));
        }
        repaint();
    }
    else if (msg->msg_type == TEXT_RECV)
    {
        if(partner.count(msg->ip) == 0) {
            Partner* p = addPartner(msg->ip);
        }
        QString str = QString::fromStdString(std::string((char*)msg->data, msg->len));  
        str = QHostAddress(msg->ip).toString()+": " + str;
        if (ui->listWidget->count() == 0) {
            QDateTime time = QDateTime::fromTime_t(QDateTime::currentDateTimeUtc().toTime_t());
            time.setTimeSpec(Qt::UTC);
            QString m_Time = time.toString("yyyy-MM-dd ddd hh:mm:ss");

            QLabel* label = new QLabel;
            label->setText(m_Time);
            label->setAlignment(Qt::AlignCenter);
            QListWidgetItem* item = new QListWidgetItem;
            ui->listWidget->addItem(item);
            ui->listWidget->setItemWidget(item, label);
            ui->listWidget->addItem(str);
        }
        else
            ui->listWidget->addItem(str);

    }

    else if (msg->msg_type == PARTNER_JOIN)
    {
        Partner* p = addPartner(msg->ip);
        if (p)
        {
            p->setpic(QImage(":/image/1"));
            ui->outlog->setText(QString("%1 join meeting").arg(QHostAddress(msg->ip).toString()));
        }
        ui->openVedio->setDisabled(false);
    }
    else if (msg->msg_type == PARTNER_EXIT)
    {
        removePartner(msg->ip);
        ui->outlog->setText(QString("%1 exit meeting").arg(QHostAddress(msg->ip).toString()));
    }
    else if (msg->msg_type == CLOSE_CAMERA)
    {
        closeImg(msg->ip);
    }
    else if (msg->msg_type == REMOTEHOSTCLOSE_ERROR)
    {

        clearPartner();
        _mytcpSocket->disconnectionFromHost();
        _mytcpSocket->wait();
        ui->outlog->setText(QString("关闭与服务器的连接"));
        ui->createmeetBtn->setDisabled(true);
        ui->exitmeetBtn->setDisabled(true);
        ui->joinmeetBtn->setDisabled(true);
        //清除聊天记录
        while (ui->listWidget->count() > 0)
        {
            ui->listWidget->clear();
        }
        
        if (_createmeet || _joinmeet) QMessageBox::warning(this, "Meeting Information", "会议结束", QMessageBox::Yes, QMessageBox::Yes);
    }
    else if (msg->msg_type == OTHERNET_ERROR)
    {
        QMessageBox::warning(NULL, "Network Error", "网络异常", QMessageBox::Yes, QMessageBox::Yes);
        clearPartner();
        _mytcpSocket->disconnectionFromHost();
        _mytcpSocket->wait();
        ui->outlog->setText(QString("网络异常......"));
    }
    if (msg->data)
    {
        free(msg->data);
        msg->data = NULL;
    }
    if (msg)
    {
        free(msg);
        msg = NULL;
    }
}

void Widget::textSend()
{
    qDebug() << "send text over";
    
    ui->sendmsg->setDisabled(false);
}

void Widget::cameraImageCapture(QVideoFrame frame)
{
    if (frame.isValid() && frame.isReadable())
    {
        QImage videoImg = QImage(frame.bits(), frame.width(), frame.height(), QVideoFrame::imageFormatFromPixelFormat(frame.pixelFormat()));

        QTransform matrix;
        matrix.rotate(180.0);

        QImage img = videoImg.transformed(matrix, Qt::FastTransformation).scaled(ui->mainshow_label->size());

        if (partner.size() > 1)
        {
            emit pushImg(img);
        }

        ui->mainshow_label->setPixmap(QPixmap::fromImage(img).scaled(ui->mainshow_label->size()));

        Partner* p = partner[m_ip];
        if (p) p->setpic(img);
    }
    frame.unmap();
}

void Widget::cameraError(QCamera::Error)
{
    QMessageBox::warning(this, "Camera error", _camera->errorString(), QMessageBox::Yes, QMessageBox::Yes);
}

void Widget::audioError(QString err)
{
    QMessageBox::warning(this, "Audio error", err, QMessageBox::Yes);
}

void Widget::speaks(QString ip)
{
    ui->outlog->setText(QString(ip + " 正在说话").toUtf8());
}

void Widget::recvIp(quint32 ip)
{
    if (partner.contains(m_ip))
    {
        Partner* p = partner[m_ip];
        p->setStyleSheet("border-width: 1px; border-style: solid; border-color:rgba(0, 0 , 255, 0.7)");
    }
    if (partner.contains(ip))
    {
        Partner* p = partner[ip];
        p->setStyleSheet("border-width: 1px; border-style: solid; border-color:rgba(255, 0 , 0, 0.7)");
    }
    ui->mainshow_label->setPixmap(QPixmap::fromImage(QImage(":/image/1").scaled(ui->mainshow_label->size())));
    ui->groupBox_2->setTitle(QHostAddress(ip).toString());
    qDebug() << ip;
}

void Widget::recvok_L(QString ip, QString port)
{
    if (_mytcpSocket->connectToServer(ip, port, QIODevice::ReadWrite))
    {
        ui->outlog->setText("成功连接到" + ip + ":" + port);
        ui->openAudio->setDisabled(true);
        ui->openVedio->setDisabled(true);
        ui->createmeetBtn->setDisabled(false);
        ui->exitmeetBtn->setDisabled(true);
        ui->joinmeetBtn->setDisabled(false);
        ui->sendmsg->setDisabled(false);
        ui->sendimg->setDisabled(false);
        m_ip = QHostAddress("0.0.0.0").toIPv4Address();
        emit send_L(true);
    }
    else
        emit send_L(false);
}

