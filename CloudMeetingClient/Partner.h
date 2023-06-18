#pragma once

#include <QLabel>

class Partner  : public QLabel
{
	Q_OBJECT

public:
	Partner(QWidget *parent=nullptr,quint32=0);
	~Partner();
	void setpic(QImage img);
private:
	quint32 ip;
	void mousePressEvent(QMouseEvent* ev)override;
	int w;
signals:
	void sendip(quint32);
};
