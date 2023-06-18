#include "register.h"
#include <qvariant.h>

#ifdef _MSC_VER
#pragma execution_character_set("utf-8")
#endif

Register::Register(QWidget *parent): QWidget(parent),ui(new Ui::RegisterClass())
{
	ui->setupUi(this);
}

Register::~Register()
{
	delete ui;
}

void Register::on_reg_clicked()
{
	if (ui->password->text() != ui->password2->text())
		ui->status->setText("passwords are not consistent");
	else if (ui->username->text().isNull() || ui->password->text().isNull()) {
		ui->status->setText("username or password is null");
	}
	else {
		ui->reg->setDisabled(true);
		ui->login->setDisabled(true);
		QString msg = ui->username->text() + "&" + ui->password->text();
		emit send_L(msg);
	}
}

void Register::on_login_clicked() {
	emit sendok();
}

void Register::recv_L(QString msg) {
	ui->reg->setDisabled(false);
	ui->login->setDisabled(false);
	ui->status->setText(msg);
}
