#include "EventLoop.h"
#include "Channel.h"
#include "mutex.h"

#include <unistd.h>
#include <pthread.h>
#include <sys/eventfd.h>
#include <signal.h>
#include <utility>

using namespace tiny_muduo;
namespace{
class IgnoreSigPipe{
public:
    IgnoreSigPipe(){
        ::signal(SIGPIPE,SIG_IGN);
    }
};
IgnoreSigPipe initObj;
}

EventLoop::EventLoop():running(false),
                        tid_(CurrentThread::tid()),
                        epoller_(new Epoller),
                        wake_fd_(::eventfd(0,EFD_NONBLOCK|EFD_CLOEXEC)),
                        wake_channel_(new Channel(this,wake_fd_)),
                        timer_queue_(new TimerQueue(this)),
                        calling_functors_(false)
{
   wake_channel_->SetReadCallback(std::bind(&EventLoop::HandleRead,this));
   wake_channel_->EnableReading(); 
}
EventLoop::~EventLoop(){
    if(running)running=false;
    wake_channel_->DisableAll();
    Remove(wake_channel_.get());
    close(wake_fd_);
}
void EventLoop::loop(){
    assert(isInLoopThread());
    running=true;
    while(running){
        active_channels_.clear();
        epoller_->Poll(active_channels_);
        for(const auto& channel:active_channels_){
            channel->HandleEvent();
        }
        DoToDoList();
    }
    running=false;
}
void EventLoop::HandleRead(){
    uint64_t read_one_byte=1;
    ssize_t read_size=::read(wake_fd_,&read_one_byte,sizeof(read_one_byte));
    (void)read_size;
    assert(read_size==sizeof(read_one_byte));
    return;
}
void EventLoop::QueueOneFunc(BasicFunc func) {
    {
        MutexLockGuard lock(mutex_);
        to_do_list_.emplace_back(std::move(func));
    }
    if(!isInLoopThread()||calling_functors_){
        uint64_t write_one_byte=1;
        ssize_t write_size=::write(wake_fd_,&write_one_byte,sizeof(write_one_byte));
        (void)write_size;
        assert(write_size==sizeof(write_one_byte));
    }
}
void EventLoop::RunOneFunc(BasicFunc func) {
  if (isInLoopThread()) {
    func();
  }else{
    QueueOneFunc(std::move(func));
  }
}
void EventLoop::DoToDoList()
{
    ToDoList functors;
    calling_functors_=true;
    {
        MutexLockGuard lock(mutex_);
        functors.swap(to_do_list_);
    }
    for(const auto&func:functors){
        func();
    }
    calling_functors_=false;
};
