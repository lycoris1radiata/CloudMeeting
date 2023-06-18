#pragma once
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string>
#include <memory>
#include <utility>

#include "callback.h"
#include "Channel.h"
#include "Buffer.h"
#include "noncopyable.h"
#include "timestamp.h"
#include "address.h"

using std::string;

namespace tiny_muduo
{
class EventLoop;
class Connection:public std::enable_shared_from_this<Connection>,NonCopyAble{
public:
    enum ConnectionState{kConnecting,kConnected,kDisconnecting,kDisconnected};
    Connection(EventLoop*loop,int connfd,int id,const Address& localAddr,const Address& peerAddr);
    ~Connection();
    void SetConnectionCallback(const ConnectionCallback&callback){connection_callback_=callback;}
    void SetConnectionCallback(ConnectionCallback&& callback) { connection_callback_ = std::move(callback);}
    void SetMessageCallback(const MessageCallback& callback) {message_callback_ = callback;}
    void SetMessageCallback(MessageCallback&& callback) {message_callback_ = std::move(callback);}
    void SetCloseCallback(const CloseCallback& callback) {close_callback_ = callback;}
    void SetCloseCallback(CloseCallback&& callback) {close_callback_ = std::move(callback);}

    void ConnectionEstablished(){
        state_=kConnected;
        channel_->Tie(shared_from_this());
        channel_->EnableReading();
        connection_callback_(shared_from_this(),&input_buffer_);
    }

    void Shutdown();
    bool IsShutDown()const{return state_==kDisconnecting;}
    int GetErrno()const;
    void ConnectionDestructor();
    void HandleClose();
    void HandleError();
    void HandleMessage();
    void HandleWrite();
    void Send(Buffer* buffer);
    void Send(const string& str);
    void Send(const char* message, int len);
    void Send(const char* message) { Send(message, static_cast<int>(strlen(message))); }
    void UpdateTimestamp(Timestamp now) { timestamp_ = now; }
    Timestamp timestamp() const { return timestamp_; }
    int fd() const { return fd_; }
    int id() const { return connection_id_; }
    EventLoop* loop() const { return loop_; }
    const Address& localAddress() const { return localAddr_; }
    const Address& peerAddress() const { return peerAddr_; }
    bool kconnected(){return state_==kConnected;}
    int roomId(){return roomid_;}
    void setRoomid(int id){roomid_=id;}
private:
    EventLoop*loop_;
    int fd_;
    int connection_id_;
    const Address localAddr_;
    const Address peerAddr_;
    ConnectionState state_;
    std::unique_ptr<Channel>channel_;
    Buffer input_buffer_;
    Buffer output_buffer_;
    bool shutdown_state_;
    Timestamp timestamp_;
    ConnectionCallback connection_callback_;
    MessageCallback message_callback_;
    CloseCallback close_callback_;
    int roomid_;
};
typedef std::shared_ptr<Connection> ConnectionPtr;
} // namespace tiny_muduo;