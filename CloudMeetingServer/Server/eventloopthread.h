#pragma once

#include "thread.h"
#include "mutex.h"
#include "condition.h"
#include "noncopyable.h"

namespace tiny_muduo
{
class EventLoop;
class EventLoopThread:public NonCopyAble{
public:
    EventLoopThread();
    ~EventLoopThread();
    void StartFunc();
    EventLoop* StartLoop();
private:
    EventLoop* loop_;
    Thread thread_;
    MutexLock mutex_;
    Condition cond_;
};
} // namespace tiny_muduo
