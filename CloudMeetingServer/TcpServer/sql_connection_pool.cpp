#include <mysql/mysql.h>
#include <stdio.h>
#include <string>
#include <string.h>
#include <stdlib.h>
#include <list>
#include <pthread.h>
#include <iostream>
#include <assert.h>
#include "sql_connection_pool.h"
#include "log.h"

using namespace std;
using namespace tiny_muduo;

connection_pool::connection_pool()
{
	m_CurConn = 0;
	m_FreeConn = 0;
}

connection_pool *connection_pool::GetInstance()
{
	static connection_pool connPool;
	return &connPool;
}


void connection_pool::init(string url, string User, string PassWord, string DBName, int Port, int MaxConn)
{
	m_url = url;
	m_Port = Port;
	m_User = User;
	m_PassWord = PassWord;
	m_DatabaseName = DBName;

	for (int i = 0; i < MaxConn; i++)
	{
		MYSQL *con = NULL;
		con = mysql_init(con);
		
		if (con == NULL)
		{
			LOG_ERROR("MySQL Error");
			exit(1);
		}
		con = mysql_real_connect(con, url.c_str(), User.c_str(), PassWord.c_str(), DBName.c_str(), Port, NULL, 0);
		
		if (con == NULL)
		{
			LOG_ERROR("MySQL Error");
			exit(1);
		}
		connList.push_back(con);
		++m_FreeConn;
	}

	reserve = sem(m_FreeConn);

	m_MaxConn = m_FreeConn;
}



MYSQL *connection_pool::GetConnection()
{
	MYSQL *con = NULL;

	if (0 == connList.size())
		return NULL;

	reserve.wait();
	
	
	{
		MutexLockGuard lock(mutex_);
		con = connList.front();
		connList.pop_front();

		--m_FreeConn;
		++m_CurConn;
	}

	return con;
}


bool connection_pool::ReleaseConnection(MYSQL *con)
{
	if (NULL == con)
		return false;

	{
		MutexLockGuard lock(mutex_);
		connList.push_back(con);
		++m_FreeConn;
		--m_CurConn;
	}

	reserve.post();
	return true;
}


void connection_pool::DestroyPool()
{

	{
		if (connList.size() > 0)
		{
			MutexLockGuard lock(mutex_);
			list<MYSQL *>::iterator it;
			for (it = connList.begin(); it != connList.end(); ++it)
			{
				MYSQL *con = *it;
				mysql_close(con);
			}
			m_CurConn = 0;
			m_FreeConn = 0;
			connList.clear();
		}
	}
}


int connection_pool::GetFreeConn()
{
	return this->m_FreeConn;
}

connection_pool::~connection_pool()
{
	DestroyPool();
}

connectionRAII::connectionRAII(MYSQL **SQL, connection_pool *connPool){
	*SQL = connPool->GetConnection();
	
	conRAII = *SQL;
	poolRAII = connPool;
}

connectionRAII::~connectionRAII(){
	poolRAII->ReleaseConnection(conRAII);
}