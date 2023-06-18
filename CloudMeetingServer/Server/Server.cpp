#include "Server.h"

#include <assert.h>
#include <climits>
#include <utility>

#include "Acceptor.h"
#include "Connection.h"
#include "eventloopthreadpool.h"
#include "log.h"

using namespace tiny_muduo;

Server::Server(EventLoop *loop, const Address &address):
        loop_(loop),
        next_connection_id_(1),
        threads_(new EventLoopThreadPool(loop_)),
        acceptor_(new Acceptor(loop_,address)),
        ipport_(std::move(address.IpPortToString()))
{
    acceptor_->SetNewConnectionCallback(std::bind(&Server::HandleNewConnection,this,_1,_2,_3));
}
Server::~Server()
{
    for(auto&pair:connections_){
        ConnectionPtr ptr(pair.second);
        pair.second.reset();
        ptr->loop()->RunOneFunc(std::bind(&Connection::ConnectionDestructor,ptr));
    }
}

inline void Server::HandleClose(const ConnectionPtr &ptr)
{
    loop_->RunOneFunc(std::bind(&Server::HandleCloseInLoop,this,ptr));
}

inline void Server::HandleCloseInLoop(const ConnectionPtr &ptr)
{
    assert(connections_.find(ptr->fd())!=connections_.end());
    connections_.erase(connections_.find(ptr->fd()));
    //printf("Server:remove connection %d\n",ptr->fd());

    LOG_INFO("Server::HandleCloseInLoop - remove connection [%s#%d]\n",&*ipport_.begin(),ptr->id());
    EventLoop*loop=ptr->loop();
    loop->QueueOneFunc(std::bind(&Connection::ConnectionDestructor,ptr));

}

inline void Server::HandleNewConnection(int connfd, const Address& localaddress,const Address& peeraddress)
{
    EventLoop*loop=threads_->NextLoop();
    ConnectionPtr ptr(new Connection(loop,connfd,next_connection_id_,localaddress,peeraddress));
    connections_[connfd]=ptr;

    ptr->SetConnectionCallback(connection_callback_);
    ptr->SetMessageCallback(message_callback_);
    ptr->SetCloseCallback(std::bind(&Server::HandleClose, this, _1));
    
    LOG_INFO("Server::HandleNewConnection - new connection [%s#%d] from %s\n",&*ipport_.begin(),next_connection_id_,&*peeraddress.IpPortToString().begin());
    //printf("Server:new connection %d\n",ptr->fd());
    ++next_connection_id_;
    if(next_connection_id_==INT_MAX)next_connection_id_=1;
    loop->RunOneFunc(std::bind(&Connection::ConnectionEstablished,ptr));
}


