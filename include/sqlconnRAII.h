#pragma once

#include "sqlconnpool.h"

class SqlConnRAII
{
private:
    MYSQL* _sql;
    SqlConnPool* _connpool;
public:
    SqlConnRAII(MYSQL** sql, SqlConnPool* connpool)
    {
        assert(connpool);
        *sql = connpool->getConn();
        _sql = *sql;
        _connpool = connpool;
    }

    ~SqlConnRAII()
    {
        if (_sql) {
            _connpool->freeConn(_sql);
        }
    }
};
