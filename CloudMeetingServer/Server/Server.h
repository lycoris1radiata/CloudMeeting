#pragma once
#include <memory>
#include <map>
#include <utility>
#include <memory>
#include <string>

#include "callback.h"
#include "EventLoop.h"
#include "Acceptor.h"
#include "Connection.h"
#include "noncopyable.h"
#include "address.h"
#include "eventloopthreadpool.h"

namespace tiny_muduo
{
class Address;
class Server:public NonCopyAble{
public:
    Server(EventLoop*loop,const Address&address);
    ~Server();
    void Start(){
        threads_->StartLoop();
        loop_->RunOneFunc(std::bind(&Acceptor::Listen,acceptor_.get()));
    }

    void SetConnectionCallback(ConnectionCallback&& callback) { connection_callback_ = std::move(callback);}
    void SetConnectionCallback(const ConnectionCallback& callback) { connection_callback_ = callback;}
    void SetMessageCallback(MessageCallback&& callback) {message_callback_ = std::move(callback);}
    void SetMessageCallback(const MessageCallback& callback) {message_callback_ = callback;}
    void SetThreadNums(int thread_nums) {threads_->SetThreadNums(thread_nums);}

    inline void HandleClose(const ConnectionPtr&conn);
    inline void HandleCloseInLoop(const ConnectionPtr& ptr);
    inline void HandleNewConnection(int connfd, const Address& localaddress,const Address& peeraddress);

private:
    typedef std::map<int,ConnectionPtr> ConnectionMap;
    EventLoop*loop_;
    int next_connection_id_;
    std::unique_ptr<EventLoopThreadPool> threads_;
    std::unique_ptr<Acceptor> acceptor_;
    const std::string ipport_;
    ConnectionCallback connection_callback_;
    MessageCallback message_callback_;
    ConnectionMap connections_;

};
} // namespace tiny_muduo
