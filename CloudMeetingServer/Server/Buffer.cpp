#include "Buffer.h"
#include <sys/uio.h>
#include <iostream>
#include <unistd.h>

#include "log.h"

int tiny_muduo::Buffer::ReadFd(int fd)
{
    char extrabuf[65536] = {0};
    struct iovec iv[2];
    const int writable = writablebytes();
    iv[0].iov_base = beginwrite();
    iv[0].iov_len = writable;
    iv[1].iov_base = extrabuf;
    iv[1].iov_len = sizeof(extrabuf);
    const int iovcnt = (writable < static_cast<int>(sizeof(extrabuf)) ? 2 : 1);
    int readn = static_cast<int>(::readv(fd, iv, iovcnt));
    if (readn < 0)
    {
        LOG_WARN("%s\n","Buffer::ReadFd readv failed");
    }
    else if (readn <= writable)
    {
        write_index_ += readn;
    }
    else
    {
        write_index_ = static_cast<int>(buffer_.size());
        Append(extrabuf, readn - writable);
    }

    //std::string tmp(beginread(),beginread()+readablebytes());
    //std::cout<<tmp<<std::endl;

    return readn;
}

ssize_t tiny_muduo::Buffer::Readn(int fd, void *buf, size_t size)
{
    ssize_t willread=size,hasread=0;
    char* ptr=(char*)buf;
    while(willread>0){
        if((hasread=read(fd,ptr,willread))<0){
            if(errno==EINTR||EAGAIN||EWOULDBLOCK)
                hasread=0;
            else{
                LOG_WARN("Buffer::Readn read failed:errno--%d\n",errno);
                return -1;
            }
        }
        else if(hasread==0)
            break;
        willread-=hasread;
        ptr+=hasread;
    }
    return size-willread;
}

ssize_t tiny_muduo::Buffer::Writen(int fd, const void *buf, size_t size)
{
    ssize_t willwrite=size,haswrite=0;
    char*ptr=(char*)buf;
    while(willwrite>0){
        if((haswrite=write(fd,ptr,willwrite))<0){
            int i=errno;
            if(errno==EINTR)
                haswrite=0;
            else{
                LOG_WARN("Buffer::Writen write failed:errno--%d\n",errno);
                return -1;
            }
        }
        willwrite-=haswrite;
        ptr+=haswrite;
    }
    return size;
}
