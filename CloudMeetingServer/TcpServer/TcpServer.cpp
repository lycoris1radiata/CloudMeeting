#include "TcpServer.h"
#include <mysql/mysql.h>
#include "MsgDef.h"
#include "timestamp.h"
#include "mutex.h"
#include <sys/socket.h>

#include <fstream>

#define IMAGE_PATH "./image/"
#define TCP_BUFSIZ 8192

using std::string;


tiny_muduo::TcpServer::TcpServer(EventLoop *loop, const Address &address, bool auto_close_idleconnection): 
                loop_(loop), 
                server_(loop, address), 
                auto_close_idleconnection_(auto_close_idleconnection)
{
    server_.SetConnectionCallback(std::bind(&TcpServer::ConnectionCallback, this, _1));
    server_.SetMessageCallback(std::bind(&TcpServer::MessageCallback, this, _1, _2));
    LOG_INFO ("TcpServer listening on %s : %d\n",address.ip(),address.port());
    sqlconns_ = connection_pool::GetInstance();
    sqlconns_->init("localhost", "root", "zzx12345", "yourdb", 3306, 8);
}

tiny_muduo::TcpServer::~TcpServer()
{
}

void tiny_muduo::TcpServer::Start()
{
    server_.Start();
}

void tiny_muduo::TcpServer::MessageCallback(const ConnectionPtr &connection, Buffer *buffer)
{
    if (auto_close_idleconnection_)
        connection->UpdateTimestamp(Timestamp::Now());
    if (connection->IsShutDown())
        return;
    
    string head(11,'0');
    MSG msg;
    int ret=buffer->Readn(connection->fd(),&*head.begin(),11);

    if(ret<=0){
        char sendbuf[12]={0};
        int len = 0;
        sendbuf[len++] = '$';
        unsigned short type = htons((unsigned short)PARTNER_EXIT);
        memcpy(&sendbuf[len], &type, sizeof(short));
        len+=2;
        unsigned int ip=connection->peerAddress().ipton();
        memcpy(&sendbuf[len], &ip, sizeof(uint32_t));
        len+=4;
        
        len+=4;

        sendbuf[len++] = '#';

        int roomid=connection->roomId();
        std::set<int> room=roomsBroadcast_(roomid,connection->fd());
        if(room.size()!=0){
            for(auto it=room.begin();it!=room.end();it++){                
                if(buffer->Writen(*it, sendbuf, len) < 0)
                {
                    LOG_ERROR("writen fd--%d error\n",*it);
                }
            }
        }
        connection->HandleClose();

        return;
    }
    else if(ret==11){

        if(head[0]=='$'){
            memset(&msg,0,sizeof(MSG));

            MSG_TYPE msgtype;
            memcpy(&msgtype,&head[1],2);
            msgtype=(MSG_TYPE)ntohs(msgtype);

            unsigned int ip;
            memcpy(&ip,&head[3],4);
            msg.ip=ntohl(ip);

            unsigned int msglen;
            memcpy(&msglen,&head[7],4);
            msg.len=ntohl(msglen);
            LOG_INFO("receive header data:msgtype--%d,data len--%d\n",msgtype,msg.len);

            if(msgtype == IMG_SEND || msgtype == AUDIO_SEND || msgtype == TEXT_SEND)
            {
                msg.msgType = (msgtype == IMG_SEND) ? IMG_RECV : ((msgtype == AUDIO_SEND)? AUDIO_RECV : TEXT_RECV);
                msg.ptr=(char*)malloc(msg.len+1);
                if(msg.ptr==nullptr){
                    LOG_ERROR("malloc error\n");
                    return;
                }
                memset(msg.ptr,0,msg.len+1);
                if((ret = buffer->Readn(connection->fd(), msg.ptr, msg.len)) < msg.len)
                {
                    LOG_ERROR ("buffer Readn data too short\n");
                    if(msg.ptr)
                        free(msg.ptr);
                    return;
                }
                else
                {
                    char tail;
                    buffer->Readn(connection->fd(), &tail, 1);
                    if(tail != '#')
                    {
                        LOG_ERROR("msg format not # error");
                        if(msg.ptr)
                            free(msg.ptr);
                        return;
                    }
                }
            }
            else if(msgtype == CLOSE_CAMERA)
            {
                char tail;
                buffer->Readn(connection->fd(), &tail, 1);
                if(tail == '#' && msg.len == 0)
                {
                    msg.msgType = CLOSE_CAMERA;
                }
                else
                {
                    LOG_ERROR("camera data error ");
                    return;
                }
            }
            else if(msgtype==CREATE_MEETING){
                char tail;
                buffer->Readn(connection->fd(), &tail, 1);
                if(msg.len == 0 && tail == '#')
                {
                    char *c = (char *)&msg.ip;
                    LOG_INFO("create meeting  ip: %d.%d.%d.%d\n", (u_char)c[3], (u_char)c[2], (u_char)c[1], (u_char)c[0]);
                    LOG_INFO("server find peer ipport:%s\n",&*connection->peerAddress().IpPortToString().begin());

                    msg.ptr = (char *) malloc(sizeof(int));
                    if(msg.ptr==nullptr){
                        LOG_ERROR("CREATE_MEETING malloc error\n");
                        return;
                    }
                    memset(msg.ptr,0,sizeof(int));
                    bool b=roomsOperate(connection->roomId(),connection->fd(),true);
                    if(!b)
                    {                    
                        msg.msgType = CREATE_MEETING_RESPONSE;
                        int roomNo = 0;
                        LOG_INFO("not room\n");
                        memcpy(msg.ptr, &roomNo, sizeof(int));
                        msg.len = sizeof(roomNo);
                    }
                    else
                    {
                        msg.msgType = CREATE_MEETING_RESPONSE;
                        int roomNo = connection->fd();
                        connection->setRoomid(connection->fd());
                        LOG_INFO("create room id:%d\n",roomNo);
                        memcpy(msg.ptr, &roomNo, sizeof(int));
                        msg.len = sizeof(roomNo);

                    }
                }
                else{
                    LOG_ERROR("CREATE_MEETING data format # or 0 error\n");
                    return;
                }

            }
            else if(msgtype == JOIN_MEETING){
                uint32_t msgsize=msg.len, roomno;
                char data[msgsize + 1]={0};
                int r = buffer->Readn(connection->fd(), data, msgsize + 1 );
                if(r < msgsize + 1)
                {
                    LOG_ERROR("JOIN_MEETING data too short\n");
                    return;
                }
                else
                {
                    if(data[msgsize] == '#')
                    {
                        memcpy(&roomno, data, msgsize);
                        roomno = ntohl(roomno);
                        uint32_t ok=0;
                        bool b=roomsOperate(roomno,connection->fd(),false);
                        if(b){
                            ok=1;
                            connection->setRoomid(roomno);
                        }
                        LOG_INFO("JOIN_MEETING find room id--%d statue is %d\n",roomno,ok);
                        msg.msgType = JOIN_MEETING_RESPONSE;
                        msg.len = sizeof(uint32_t);
                        msg.ptr = (char *)malloc(sizeof(uint32_t));
                        if(msg.ptr==nullptr){
                            LOG_ERROR("JOIN_MEETING malloc error\n");
                            return;
                        }
                        memset(msg.ptr,0,sizeof(uint32_t));
                        memcpy(msg.ptr, &ok, sizeof(uint32_t));
                    }
                    else
                    {
                        LOG_ERROR("JOIN_MEETING data format # error\n");
                        return;
                    }
                }
            }
            else if(msgtype == LOGIN||msgtype ==REGISTER)
            {
                msg.msgType= msgtype == LOGIN ? LOGIN_RESPONSE:REGISTER_RESPONSE;
                bool islogin= msgtype == LOGIN ? true:false;
                uint32_t msgsize=msg.len;
                string userdata(msgsize+1,'0');
                if((ret = buffer->Readn(connection->fd(), &*userdata.begin(), msgsize+1)) < msgsize+1)
                {
                    LOG_ERROR ("buffer Readn data too short\n");
                    return;
                }
                else{
                    string res;
                    if(userdata[msgsize]=='#')
                    {
                        res=SqlOperate(userdata.substr(0,msgsize),islogin,connection);
                        msg.len=res.size();
                        msg.ptr=(char *)malloc(msg.len);
                        if(msg.ptr==nullptr){
                            LOG_ERROR("LOGIN malloc error\n");
                            return;
                        }
                        memset(msg.ptr,0,msg.len);
                        memcpy(msg.ptr, res.data(), msg.len);
                    }
                    else
                    {
                        LOG_ERROR("LOGIN data format # error\n");
                        return;
                    }
                }
            }
            else{
                LOG_ERROR("msgtype error\n");
                return;
            }
        }
        else{
            LOG_ERROR("msg format $ error");
            return;
        }
    }
    else{
        LOG_ERROR("msg header error");
        return;
    }

    string sendbuf(4 * 1024*1024,'0');
    int len = 0;
    sendbuf[len++] = '$';

    unsigned short type = htons((unsigned short)msg.msgType);
    memcpy(&sendbuf[len], &type, sizeof(short));
    len+=2;

    if(msg.msgType == CREATE_MEETING_RESPONSE || msg.msgType == JOIN_MEETING_RESPONSE)
    {
        unsigned int ip=Address("0.0.0.0",80).ipton();
        memcpy(&sendbuf[len], &ip, sizeof(uint32_t));
        len += 4;
    }
    else if(msg.msgType == TEXT_RECV || msg.msgType == IMG_RECV || msg.msgType == AUDIO_RECV || msg.msgType == CLOSE_CAMERA||msg.msgType==LOGIN_RESPONSE||msg.msgType==REGISTER_RESPONSE)
    {
        unsigned int ip=htonl(msg.ip);
        memcpy(&sendbuf[len], &ip, sizeof(uint32_t));
        len+=4;
    }

    unsigned int msglen = htonl(msg.len);
    memcpy(&sendbuf[len], &msglen, sizeof(int));
    len += 4;

    if(msg.len!=0){
        memcpy(&sendbuf[len], msg.ptr, msg.len);
        len += msg.len;
    }

    sendbuf[len++] = '#';

    if(msg.msgType == CREATE_MEETING_RESPONSE||msg.msgType==LOGIN_RESPONSE||msg.msgType==REGISTER_RESPONSE)
    {
        if(buffer->Writen(connection->fd(), &*sendbuf.begin(), len) < 0)
        {
            LOG_ERROR("writen error");
        }
    }
    else if(msg.msgType ==JOIN_MEETING_RESPONSE)
    {
        if(buffer->Writen(connection->fd(), &*sendbuf.begin(), len) < 0)
        {
            LOG_ERROR("writen error");
        }
        else{
            unsigned short broadcast_msgtype=htons(PARTNER_JOIN);
            memcpy(&sendbuf[1],&broadcast_msgtype,sizeof(short));

            unsigned int ip=htonl(msg.ip);
            memcpy(&sendbuf[3], &ip, sizeof(uint32_t));

            int roomid=connection->roomId();
            std::set<int> room=roomsBroadcast_(roomid,connection->fd());
            if(room.size()!=0){
                for(auto it=room.begin();it!=room.end();it++){                
                    if(buffer->Writen(*it, &*sendbuf.begin(), len) < 0)
                    {
                        LOG_ERROR("writen fd--%d error\n",*it);
                    }
                }
            }

        }
    }
    else if(msg.msgType == IMG_RECV || msg.msgType == AUDIO_RECV || msg.msgType == TEXT_RECV || msg.msgType == CLOSE_CAMERA)
    {
        int roomid=connection->roomId();
        std::set<int> room=roomsBroadcast_(roomid,connection->fd());
        if(room.size()!=0){
            for(auto it=room.begin();it!=room.end();it++){                
                if(buffer->Writen(*it, &*sendbuf.begin(), len) < 0)
                {
                    LOG_ERROR("writen fd--%d error\n",*it);
                }
            }
        }
    }
    else{
        LOG_ERROR("msgtype error");
    }
    if(msg.ptr)
    {
        free(msg.ptr);
        msg.ptr = nullptr;
    }
}

void tiny_muduo::TcpServer::ConnectionCallback(const ConnectionPtr& connection){
    if (auto_close_idleconnection_) {
    loop_->RunAfter(kIdleConnectionTimeOuts, 
                    std::bind(&TcpServer::HandleIdleConnection, 
                    this, std::weak_ptr<Connection>(connection))); 
    }
}

string tiny_muduo::TcpServer::SqlOperate(string userdata,bool islogin,const ConnectionPtr& connection)
{
    MYSQL* mysql=NULL;
    connectionRAII mysqlcon(&mysql, sqlconns_);

    std::vector<string> udata;

    auto end=std::find(userdata.begin(),userdata.end(),'&');
    auto start=userdata.begin();
    while(end!=userdata.end()){
        udata.push_back({start,end});
        start=end+1;
        end=std::find(start,userdata.end(),'&');
    }
    udata.push_back({start,end});
    if(udata.size()<2)
        return "Username or password is null!";

    string query="SELECT username,passwd FROM user WHERE username='"+udata[0]+"'";
    if (mysql_query(mysql, &*query.begin()))
    {
        LOG_ERROR("SELECT error:%s\n", mysql_error(mysql));
    }
    MYSQL_RES *result = mysql_store_result(mysql);
    MYSQL_ROW row = mysql_fetch_row(result);
    if(islogin){
        if (row)
        {
            if(string(row[1])==udata[1]){
                return "login success";
            }
            else 
                return "password error";
        }
        else 
            return "username dont exist";
    }
    else{
        if (row)
            return "username existed";
        else{
            string cmd="INSERT INTO user(username, passwd) VALUES('"+udata[0]+"', '"+udata[1]+"')";
            LOG_INFO("%s\n",&*cmd.begin());
            if (mysql_query(mysql, &*cmd.begin()))
            {
                LOG_ERROR("SELECT error:%s\n", mysql_error(mysql));
            }
            return "register success";
        }
    }
}

bool tiny_muduo::TcpServer::roomsOperate(int roomid,int fd,bool isCreate){
    bool ret;
    if(isCreate){
        MutexLockGuard locker(mutex_);
        if(rooms_.count(fd)){
            rooms_[fd].clear();
            rooms_[fd].insert(fd);
        }
        else{  
            std::set<int> r{fd};
            rooms_.insert(make_pair(fd,r));
        }
        ret=true;        
    }
    else{
        MutexLockGuard locker(mutex_);
        if(rooms_.count(roomid)){
            rooms_[roomid].insert(fd);
            ret=true;
        }
        else{
            ret=false;
        }
    }
    return ret;
}

std::set<int> tiny_muduo::TcpServer::roomsBroadcast_(int roomid,int fd){
    std::set<int> ret;
    {
        MutexLockGuard locker(mutex_);
        if(rooms_.count(roomid)){
            for(auto it=rooms_[roomid].begin();it!=rooms_[roomid].end();it++){
                if(*it!=fd)
                    ret.insert(*it);
            }
        }
    }
    return ret;
}