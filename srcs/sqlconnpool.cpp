#include "sqlconnpool.h"


SqlConnPool::SqlConnPool()
{
    _userCount = 0;
    _freeCount = 0;
}

SqlConnPool::~SqlConnPool()
{
    closePool();
}

SqlConnPool* SqlConnPool::Instance()
{
    static SqlConnPool connPool;
    return &connPool;
}

MYSQL* SqlConnPool::getConn()
{
    MYSQL* sql = nullptr;
    if (_connQue.empty()) {
        LOG_WARN("SqlConnQool busy!");
        return nullptr;
    }
    sem_wait(&_sem);
    {
        std::lock_guard<std::mutex> locker(_mtx);
        sql = _connQue.front();
        _connQue.pop();
    }
    return sql;
}

void SqlConnPool::freeConn(MYSQL* sqlconn)
{
    assert(sqlconn);
    std::lock_guard<std::mutex> locker(_mtx);
    _connQue.push(sqlconn);
    sem_post(&_sem);
}

int SqlConnPool::getFreeConnCount()
{
    std::lock_guard<std::mutex> locker(_mtx);
    return _connQue.size();
}

void SqlConnPool::init(const char* host, int port, const char* user, const char* pwd, const char* dbname, int connSize)
{
    assert(connSize > 0);
    for (int i = 0; i < connSize; ++i) {
        MYSQL* sql = nullptr;
        sql = mysql_init(sql);
        if (!sql) {
            LOG_ERROR("MySql init error!");
            assert(sql);
        }
        sql = mysql_real_connect(sql, host, user, pwd, dbname, port, nullptr, 0);
        if (!sql) {
            LOG_ERROR("MySql connect error!");
        }
        _connQue.push(sql);
    }
    _maxConn = connSize;
    sem_init(&_sem, 0, _maxConn);
}

void SqlConnPool::closePool()
{
    std::lock_guard<std::mutex> locker(_mtx);
    while(!_connQue.empty()) {
        auto sql = _connQue.front();
        _connQue.pop();
        mysql_close(sql);
    }
    mysql_library_end();
}
