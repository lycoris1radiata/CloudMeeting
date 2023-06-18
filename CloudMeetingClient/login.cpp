#include "login.h"
#include <qhostaddress.h>
#include "NetHeader.h"
#include <qdebug.h>
#include <QtEndian>
#include <qregexp.h>
#include <qmessagebox.h>
#include <qinputdialog.h>


#ifdef _MSC_VER
#pragma execution_character_set("utf-8")
#endif

extern QUEUE_DATA<MESG> queue_send;

Login::Login(QWidget*parent): QWidget(parent),ui(new Ui::LoginClass())
{
	ui->setupUi(this);
	qRegisterMetaType<MSG_TYPE>();
	ui->username->setText("zz");
	ui->password->setText("zzx12345");
	ui->login->setDisabled(false);
	ui->reg->setDisabled(false);
	ip_ = "192.168.1.128";
	port_ = "8888";
	ui->status->setText("server ipport need to setup!");

	regui = new Register;
	mainWidget = new Widget;
	_mytcpSocket = new TcpSock;

	connect(this, &Login::sendok_W, mainWidget, &Widget::recvok_L);
	connect(mainWidget, &Widget::send_quit, this, [=]() {
		show();
		mainWidget->hide();
		ui->status->setText("quit success");
		});
	connect(mainWidget, &Widget::send_L, this, [=](bool isnet) {
		if (isnet) {
			hide();
			mainWidget->show();
		}
		else {
			ui->status->setText("network is disconnected");
		}
		});
	connect(_mytcpSocket, &TcpSock::send_L, this, &Login::dataDeal);

	regui->hide();

	connect(regui, &Register::sendok, this, [=]() {
		show();
		regui->hide();
		});
	connect(regui, SIGNAL(send_L(QString)),this,SLOT(recv_R(QString)));
}

Login::~Login()
{
	if (_mytcpSocket->isRunning()) {
		_mytcpSocket->stopImmediately();
		_mytcpSocket->wait();
	}
	delete _mytcpSocket;
	delete regui;
	delete mainWidget;
	delete ui;
}

void Login::on_login_clicked() {
	if (ui->username->text().isNull() || ui->password->text().isNull()) {
		ui->status->setText("username or password is null");
		return;
	}
	ui->login->setDisabled(true);
	if (_mytcpSocket->connectToServer(ip_, port_)) {
		QString msg = ui->username->text() + "&" + ui->password->text();
		MESG* send = (MESG*)malloc(sizeof(MESG));
		if (send == nullptr)
		{
			qDebug() << __FILE__ << __LINE__ << "LOGIN malloc fail";
		}
		else {
			send->msg_type = LOGIN;
			send->len = msg.size();
			if (send->len>0) {
				send->data = (uchar*)malloc(send->len);
				if (send->data == nullptr)
				{
					qDebug() << __FILE__ << __LINE__ << "LOGIN send.data malloc error";
					free(send);
				}
				else {
					memset(send->data, 0, send->len);
					memcpy(send->data, msg.toUtf8().data(), send->len);
					queue_send.push_msg(send);
				}
			}
		}
	}
	else {
		ui->status->setText("network is disconnected");
		ui->login->setDisabled(false);
	}
}
void Login::on_reg_clicked() {
	hide();
	regui->show();
}

void Login::on_ipport_clicked() {
	bool ok = false;
	QString text = QInputDialog::getText(this, "·þÎñÆ÷ÉèÖÃ", "IP and PORT£º", QLineEdit::Normal, "192.168.1.128:1234", &ok);
	int i = text.indexOf(":");
	QString ip = text.mid(0, i);
	QString port = text.mid(i + 1, text.size() - i - 1);
	QRegExp ipreg("((2{2}[0-3]|2[01][0-9]|1[0-9]{2}|0?[1-9][0-9]|0{0,2}[1-9])\\.)((25[0-5]|2[0-4][0-9]|[01]?[0-9]{0,2})\\.){2}(25[0-5]|2[0-4][0-9]|[01]?[0-9]{1,2})");
	QRegExp portreg("^([0-9]|[1-9]\\d|[1-9]\\d{2}|[1-9]\\d{3}|[1-5]\\d{4}|6[0-4]\\d{3}|65[0-4]\\d{2}|655[0-2]\\d|6553[0-5])$");
	QRegExpValidator ipvalidate(ipreg), portvalidate(portreg);
	int pos = 0;
	if (ipvalidate.validate(ip, pos) != QValidator::Acceptable)
	{
		QMessageBox::warning(this, "Input Error", "Ip Error", QMessageBox::Yes, QMessageBox::Yes);
		return;
	}
	if (portvalidate.validate(port, pos) != QValidator::Acceptable)
	{
		QMessageBox::warning(this, "Input Error", "Port Error", QMessageBox::Yes, QMessageBox::Yes);
		return;
	}
	ip_ = ip;
	port_ = port;
	ui->status->setText("server ipport:" + text);
}

void Login::recv_R(QString msg)
{
	MESG* send = (MESG*)malloc(sizeof(MESG));
	if (send == nullptr)
	{
		qDebug() << __FILE__ << __LINE__ << "REGISTER malloc fail";
	}
	else {
		send->msg_type = REGISTER;
		send->len = msg.size();
		if (send->len > 0) {
			send->data = (uchar*)malloc(send->len);
			if (send->data == nullptr)
			{
				qDebug() << __FILE__ << __LINE__ << "malloc error";
				free(send);
			}
			else {
				memset(send->data, 0, send->len);
				queue_send.push_msg(send);
			}
		}
	}
}

void Login::dataDeal(MESG* msg)
{
	QString recvmsg=QString::fromLatin1((char*)msg->data, msg->len);
	if (msg->msg_type == LOGIN_RESPONSE)
	{
		ui->login->setDisabled(false);
		if (recvmsg.contains("login success")) {
			_mytcpSocket->disconnectionFromHost();
			_mytcpSocket->wait();
			emit sendok_W(ip_,port_);
		}
		ui->status->setText(recvmsg);
	}
	else if (msg->msg_type == REGISTER_RESPONSE)
	{
		connect(this, &Login::send_R, regui, &Register::recv_L);
		emit send_R(recvmsg);
	}
	else
		qDebug() << "LOGIN msgtype error";
}
