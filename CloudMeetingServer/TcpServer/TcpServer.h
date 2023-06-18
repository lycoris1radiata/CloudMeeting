#pragma once

#include <stdio.h>
#include <functional>
#include <utility>
#include <memory>
#include <set>
#include <map>
#include <string>

#include "callback.h"
#include "EventLoop.h"
#include "Server.h"
#include "Connection.h"
#include "Buffer.h"
#include "timestamp.h"
#include "sql_connection_pool.h"
#include "mutex.h"

namespace tiny_muduo
{
static const double kIdleConnectionTimeOuts=8.0;


class TcpServer{
    typedef std::map<string,ConnectionPtr> ConnMap;
    typedef std::set<ConnectionPtr> ConnSet;

public:
    TcpServer(EventLoop* loop,const Address& address,bool auto_close_idleconnection=false);
    ~TcpServer();

    void Start();
    
    void HandleIdleConnection(std::weak_ptr<Connection>& connection){
        ConnectionPtr conn(connection.lock());
        if(conn){
            if(Timestamp::AddTime(conn->timestamp(),kIdleConnectionTimeOuts)<Timestamp::Now()){
                conn->Shutdown();
            }else{
                loop_->RunAfter(kIdleConnectionTimeOuts, 
                            std::move(std::bind(&TcpServer::HandleIdleConnection,
                            this, connection)));
            }
        }
    }
    void ConnectionCallback(const ConnectionPtr& connection);

    void MessageCallback(const ConnectionPtr& connection, Buffer* buffer); 
    
    void SetThreadNums(int thread_nums) {server_.SetThreadNums(thread_nums);}

    std::string SqlOperate(string userdata,bool islogin,const ConnectionPtr& connection);

    bool roomsOperate(int roomid,int fd,bool isCreate);
    std::set<int> roomsBroadcast_(int roomid,int fd);


private:
    EventLoop* loop_;
    Server server_;
    bool auto_close_idleconnection_;
    std::map<int,std::set<int>> rooms_;
    connection_pool *sqlconns_;
    MutexLock mutex_;
};
} 
