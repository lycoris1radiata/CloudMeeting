#pragma once

#include <QWidget>
#include "ui_register.h"

QT_BEGIN_NAMESPACE
namespace Ui { class RegisterClass; };
QT_END_NAMESPACE

class Register : public QWidget
{
	Q_OBJECT

public:
	Register(QWidget *parent = nullptr);
	~Register();

private:
	Ui::RegisterClass *ui;
public slots:
	void on_reg_clicked();
	void on_login_clicked();
	void recv_L(QString msg);
signals:
	void send_L(QString msg);
	void sendok();
};
