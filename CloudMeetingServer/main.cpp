#include <iostream>
#include <stdio.h>
#include <string>
#include <functional>
#include <memory>
#include <climits>
#include <map>
#include <string.h>

#include "EventLoop.h"
#include "Server.h"
#include "address.h"
#include "mutex.h"
#include "log.h"
#include "TcpServer.h"

using namespace tiny_muduo;

int close_log=0;

int main()
{
    Log::get_instance()->init("./LOGFILE/",2000,800000,0);
    EventLoop loop;
    Address listen_addr(1234);
    TcpServer server(&loop,listen_addr,false);
    server.SetThreadNums(4);
    server.Start();
    loop.loop();
    return 0;
}