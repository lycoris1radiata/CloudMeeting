#pragma once 
#include <sys/epoll.h>
#include <utility>
#include <memory>

#include "EventLoop.h"
#include "callback.h"
#include "noncopyable.h"

namespace tiny_muduo
{
enum ChannelState{kNew,kAdded,kDeleted};

class Channel:public NonCopyAble{
public:
    typedef std::function<void()> ErrorCallback;
    Channel(EventLoop* loop,const int& fd);
    ~Channel();
    void HandleEvent();
    void HandleEventWithGuard();
    void SetReadCallback(ReadCallback&&callback){read_callback_=std::move(callback);}
    void SetReadCallback(const ReadCallback&callback){read_callback_=callback;}
    void SetWriteCallback(WriteCallback&&callback){write_callback_=std::move(callback);}
    void SetWriteCallback(const WriteCallback&callback){write_callback_=callback;}
    void SetErrorCallback(ErrorCallback&& callback) {error_callback_ = std::move(callback);}
    void SetErrorCallback(const ErrorCallback& callback) {error_callback_ = callback;}
    void Tie(const std::shared_ptr<void>& ptr){tied_=true;tie_=ptr;}
    void EnableReading(){events_|=(EPOLLIN|EPOLLPRI);Update();}
    void EnableWriting(){events_|=(EPOLLOUT);Update();}
    void DisableAll(){events_=0;Update();}
    void DisableWriting(){events_&=~EPOLLOUT;Update();}
    void Update(){loop_->Update(this);}
    void SetReceivedEvents(int events){recv_events_=events;}
    void SetChannelState(ChannelState state){state_=state;}
    int fd()const{return fd_;}
    int events()const{return events_;}
    int recv_events()const{return recv_events_;}
    ChannelState state()const{return state_;}
    bool IsWriting()const{return events_&EPOLLOUT;}
    bool IsReading()const{return events_&(EPOLLIN|EPOLLPRI);}
private:
    EventLoop*loop_;
    int fd_;
    int events_;
    int recv_events_;
    std::weak_ptr<void> tie_;
    bool tied_;
    int errno_;
    ChannelState state_;
    ReadCallback read_callback_;
    WriteCallback write_callback_;
    ErrorCallback error_callback_;
};

} // namespace tiny_muduo
