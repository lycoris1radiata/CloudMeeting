#pragma once

#include <stdio.h>
#include <list>
#include <mysql/mysql.h>
#include <error.h>
#include <string.h>
#include <iostream>
#include <string>

#include "sem.h"
#include "mutex.h"

using namespace std;

namespace tiny_muduo{

class connection_pool
{
public:
	MYSQL *GetConnection();				 
	bool ReleaseConnection(MYSQL *conn); 
	int GetFreeConn();					 
	void DestroyPool();					 

	
	static connection_pool *GetInstance();

	void init(string url, string User, string PassWord, string DataBaseName, int Port, int MaxConn); 

private:
	connection_pool();
	~connection_pool();

	int m_MaxConn;  
	int m_CurConn;  
	int m_FreeConn; 
	MutexLock mutex_;
	list<MYSQL *> connList; 
	sem reserve;

public:
	string m_url;			 
	string m_Port;		 
	string m_User;		 
	string m_PassWord;	 
	string m_DatabaseName; 
};

class connectionRAII{

public:
	connectionRAII(MYSQL **con, connection_pool *connPool);
	~connectionRAII();
	
private:
	MYSQL *conRAII;
	connection_pool *poolRAII;
};
}