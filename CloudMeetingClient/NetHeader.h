#pragma once

#include <qmetatype.h>
#include <qmutex.h>
#include <qwaitcondition.h>
#include <qimage.h>
#include <qqueue.h>

#define QUEUE_SIZE 1500
#define MB 1024*1024
#define KB 1024
#define WAITSECONDS 2
#define OPENVIDEO "打开视频"
#define CLOSEVIDEO "关闭视频"
#define OPENAUDIO	"打开音频"
#define CLOSEAUDIO	"关闭音频"
#define MSG_HEADER 11

enum MSG_TYPE
{
	IMG_SEND=0,
	IMG_RECV,
	AUDIO_SEND,
	AUDIO_RECV,
	TEXT_SEND,
	TEXT_RECV,
	CREATE_MEETING,
	EXIT_MEETING,
	JOIN_MEETING,
	CLOSE_CAMERA,

	LOGIN=11,
	REGISTER,
	LOGIN_RESPONSE,
	REGISTER_RESPONSE,

	CREATE_MEETING_RESPONSE=20,
	PARTNER_EXIT,
	PARTNER_JOIN,
	JOIN_MEETING_RESPONSE=23,
	REMOTEHOSTCLOSE_ERROR=40,
	OTHERNET_ERROR=41,
};
Q_DECLARE_METATYPE(MSG_TYPE);

struct MESG
{
	MSG_TYPE msg_type;
	uchar* data;
	long len;
	quint32 ip;
};
Q_DECLARE_METATYPE(MESG*);

template<class T>
struct QUEUE_DATA
{
private:
	QMutex block_queueLock;
	QWaitCondition block_queueCond;
	QQueue<T*> block_queue;
public:
	void push_msg(T* msg) {
		block_queueLock.lock();
		while (block_queue.size() > QUEUE_SIZE) {
			block_queueCond.wait(&block_queueLock);
		}
		block_queue.push_back(msg);
		block_queueLock.unlock();
		block_queueCond.wakeOne();
	}
	T* pop_mag() {
		block_queueLock.lock();
		while (block_queue.size() == 0) {
			bool f = block_queueCond.wait(&block_queueLock, WAITSECONDS * 1000);
			if (f == false) {
				block_queueLock.unlock();
				return nullptr;
			}
		}
		T* msg = block_queue.front();
		block_queue.pop_front();
		block_queueLock.unlock();
		block_queueCond.wakeOne();
		return msg;
	}
	void clear() {
		block_queueLock.lock();
		block_queue.clear();
		block_queueLock.unlock();
	}
};

