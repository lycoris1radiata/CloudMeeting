#include "Partner.h"
#include <qdebug.h>
#include <qevent.h>
#include <qhostaddress.h>

Partner::Partner(QWidget*parent,quint32 p): QLabel(parent),ip(p)
{
	setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
	w = ((QWidget*)this->parent())->size().width();
	setPixmap(QPixmap::fromImage(QImage(":/image/1").scaled(w - 10, w - 10)));
	setFrameShape(QFrame::Box);
	setStyleSheet("border-width: 1px; border-style: solid; border-color:rgba(0, 0 , 255, 0.7)");
	setToolTip(QHostAddress(ip).toString());
}

Partner::~Partner()
{}

void Partner::setpic(QImage img)
{
	setPixmap(QPixmap::fromImage(img.scaled(w - 10, w - 10)));
}

void Partner::mousePressEvent(QMouseEvent * ev)
{
	emit sendip(ip);
}
