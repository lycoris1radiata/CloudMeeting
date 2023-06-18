#pragma once
#include <iostream>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>
#include "condition.h"
#include "mutex.h"

using namespace std;

namespace tiny_muduo{

template <class T>
class block_queue
{
public:
    block_queue(int max_size = 1000):mutex_(),cond_(mutex_)
    {
        if (max_size <= 0)
        {
            exit(-1);
        }

        m_max_size = max_size;
        m_array = new T[max_size];
        m_size = 0;
        m_front = -1;
        m_back = -1;
    }

    void clear()
    {
        {
        
            MutexLockGuard lock(mutex_);
            m_size = 0;
            m_front = -1;
            m_back = -1;
        
        }
    }

    ~block_queue()
    {
        
        {
            MutexLockGuard lock(mutex_);
            if (m_array != NULL)
                delete [] m_array;
        }
        
    }
    //判断队列是否满了
    bool full() 
    {
        mutex_.Lock();
        if (m_size >= m_max_size)
        {
            mutex_.Unlock();
            return true;
        }
        mutex_.Unlock();
        return false;
    }
    //判断队列是否为空
    bool empty() 
    {
        mutex_.Lock();
        if (0 == m_size)
        {
            mutex_.Unlock();
            return true;
        }
        mutex_.Unlock();
        return false;
    }
    //返回队首元素
    bool front(T &value) 
    {
        mutex_.Lock();
        if (0 == m_size)
        {
            mutex_.Unlock();
            return false;
        }
        value = m_array[m_front];
        mutex_.Unlock();
        return true;
    }
    //返回队尾元素
    bool back(T &value) 
    {
        mutex_.Lock();
        if (0 == m_size)
        {
            mutex_.Unlock();
            return false;
        }
        value = m_array[m_back];
        mutex_.Unlock();
        return true;
    }

    int size() 
    {
        int tmp = 0;
        {
            MutexLockGuard lock(mutex_);
            tmp = m_size;

        }
        return tmp;
    }

    int max_size()
    {
        int tmp = 0;

        {
            MutexLockGuard lock(mutex_);
            tmp = m_max_size;
        }
        return tmp;
    }
    //往队列添加元素，需要将所有使用队列的线程先唤醒
    //当有元素push进队列,相当于生产者生产了一个元素
    //若当前没有线程等待条件变量,则唤醒无意义
    bool push(const T &item)
    {

        mutex_.Lock();
        if (m_size >= m_max_size)
        {
            cond_.NotifyAll();
            mutex_.Unlock();
            return false;
        }

        m_back = (m_back + 1) % m_max_size;
        m_array[m_back] = item;

        m_size++;
        cond_.NotifyAll();
        mutex_.Unlock();
        return true;
    }
    //pop时,如果当前队列没有元素,将会等待条件变量
    bool pop(T &item)
    {
        mutex_.Lock();
        while (m_size <= 0)
        {
            
            if (!cond_.Wait())
            {
                mutex_.Unlock();
                return false;
            }
        }

        m_front = (m_front + 1) % m_max_size;
        item = m_array[m_front];
        m_size--;
        mutex_.Unlock();
        return true;
    }

    //增加了超时处理
    bool pop(T &item, int ms_timeout)
    {
        mutex_.Lock();
        if (m_size <= 0)
        {
            if (!cond_.WaitForFewSeconds(ms_timeout))
            {
                mutex_.Unlock();
                return false;
            }
        }

        if (m_size <= 0)
        {
            mutex_.Unlock();
            return false;
        }

        m_front = (m_front + 1) % m_max_size;
        item = m_array[m_front];
        m_size--;
        mutex_.Unlock();
        return true;
    }

private:
    MutexLock mutex_;
    Condition cond_;
    T *m_array;
    int m_size;
    int m_max_size;
    int m_front;
    int m_back;
};
}

