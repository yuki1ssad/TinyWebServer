#pragma once

#include <mysql/mysql.h>
#include <queue>
#include <mutex>
#include <semaphore.h>
#include <cassert>
#include "log.h"


class SqlConnPool
{
private:
    int _maxConn;
    int _userCount;
    int _freeCount;
    std::queue<MYSQL*> _connQue;
    std::mutex _mtx;
    sem_t _sem;
public:
    static SqlConnPool* Instance();
    SqlConnPool();
    ~SqlConnPool();

    MYSQL* getConn();
    void freeConn(MYSQL* sqlconn);
    int getFreeConnCount();
    void init(const char* host, int port, const char* user, const char* pwd, const char* dbname, int connSize);
    void closePool();
};
