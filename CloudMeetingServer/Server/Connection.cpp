#include "Connection.h"

#include <assert.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <string.h>
#include <iostream>

#include "Channel.h"
#include "Buffer.h"
#include "log.h"

using namespace tiny_muduo;

Connection::Connection(EventLoop *loop, int connfd, int id,const Address& localAddr,const Address& peerAddr):
            loop_(loop),
            fd_(connfd),
            connection_id_(id),
            localAddr_(localAddr),
            peerAddr_(peerAddr),
            state_(kConnecting),
            channel_(new Channel(loop_,fd_)),
            shutdown_state_(false),
            roomid_(0)
{
    channel_->SetReadCallback(std::bind(&Connection::HandleMessage, this));
    channel_->SetWriteCallback(std::bind(&Connection::HandleWrite, this));
    channel_->SetErrorCallback(std::bind(&Connection::HandleError, this));
}
Connection::~Connection()
{
    ::close(fd_);
}
void Connection::Shutdown() {
    state_=kDisconnecting;
    if(!channel_->IsWriting()){
        int ret=::shutdown(fd_,SHUT_WR);
        if(ret<0){
            //printf("connection:shut down failed\n");
            LOG_ERROR("%s\n","Connection::Shutdown shutdown failed");
        }
    }
}
int Connection::GetErrno() const {
  int optval;
  socklen_t optlen = static_cast<socklen_t>(sizeof optval);
  if (::getsockopt(fd_, SOL_SOCKET, SO_ERROR, &optval, &optlen) < 0) {
    return errno;
  } else {
    return optval;
  }
}
void Connection::ConnectionDestructor()
{
    if(state_==kDisconnecting||state_==kConnected){
        state_=kDisconnected;
        channel_->DisableAll();
    }
    loop_->Remove(channel_.get());
}
void Connection::HandleClose() {
    state_=kDisconnected;
    channel_->DisableAll();
    ConnectionPtr guard(shared_from_this());
    close_callback_(guard);
}

void Connection::HandleError() {
    //printf("connection:handle error\n");
    LOG_ERROR("Connection::HandleError:%d\n",GetErrno());
}

void Connection::HandleMessage() {
    message_callback_(shared_from_this(),&input_buffer_);
    
}

void Connection::HandleWrite() {
    if(channel_->IsWriting()){
        int len=output_buffer_.readablebytes();
        int remaining=len;
        int send_size=static_cast<int>(::write(fd_,output_buffer_.Peek(),remaining));
        if(send_size<0){
            assert(send_size>=0);
            if(errno!=EWOULDBLOCK){
                //printf("connection:handle write failed\n");
                LOG_ERROR("%s\n","Connection::HandleMessage write failed");
            }
            return;
        }
        remaining-=send_size;
        output_buffer_.Retrieve(send_size);
        assert(remaining<=len);
        if(!output_buffer_.readablebytes()){
            channel_->DisableWriting();
            if(state_==kDisconnecting){
                Shutdown();
            }
        }
    }
}

void Connection::Send(Buffer *buffer)
{
    if(state_==kDisconnected)return;
    Send(buffer->Peek(),buffer->readablebytes());
    buffer->RetrieveAll();
}

void Connection::Send(const string &str)
{
    if(state_==kDisconnected)return;
    Send(str.data(),static_cast<int>(str.size()));
}

void Connection::Send(const char *message, int len)
{
    // std::string tmp(message,len);
    // std::cout<<tmp<<std::endl;

    int remaining=len;
    int send_size=0;
    if(!channel_->IsWriting()&&output_buffer_.readablebytes()==0){
        send_size=static_cast<int>(::write(fd_,message,len));
        if(send_size>=0){
            remaining-=send_size;
        }else{
            if(errno!=EWOULDBLOCK){
                //printf("connection:send write failed\n");
                LOG_ERROR("%s\n","Connection::Send write failed");
            }
            return;
        }
    }
    assert(remaining<=len);
    if(remaining>0){
        output_buffer_.Append(message+send_size,remaining);
        if(!channel_->IsWriting()){
            channel_->EnableWriting();
        }
    }
}

