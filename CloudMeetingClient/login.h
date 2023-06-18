#pragma once

#include <QtWidgets/QWidget>
#include "ui_login.h"
#include <qtcpsocket.h>

#include "register.h"
#include "Widget.h"
#include "TcpSock.h"

QT_BEGIN_NAMESPACE
namespace Ui { class LoginClass; };
QT_END_NAMESPACE


class Login : public QWidget
{
	Q_OBJECT

public:
	Login(QWidget*parent = nullptr);
	~Login();

private:
	Ui::LoginClass *ui;
	TcpSock* _mytcpSocket;
	Register* regui;
	Widget* mainWidget;
	QString ip_;
	QString port_;
public slots:
	void on_login_clicked();
	void on_reg_clicked();
	void on_ipport_clicked();
	void recv_R(QString msg);
	void dataDeal(MESG* msg);
signals:
	void send_R(QString msg);
	void sendok_W(QString ip, QString port);
};
