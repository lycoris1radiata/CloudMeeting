#pragma once

#include <QObject>
#include <qaudioinput.h>
#include <qiodevice.h>
class AudioInput : public QObject
{
	Q_OBJECT
private:
	QAudioInput *audio;
	QIODevice* inputdevice;
	char* recvbuf;
public:
	AudioInput(QObject *par = 0);
	~AudioInput();
private slots:
	void onreadyRead();
	void handleStateChanged(QAudio::State);
	QString errorString();
	void setVolumn(int);
public slots:
	void startCollect();
	void stopCollect();
signals:
	void audioinputerror(QString);
};
