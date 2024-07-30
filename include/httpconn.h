#pragma once

#include <sys/types.h>
#include <sys/uio.h>     // readv/writev
#include <arpa/inet.h>   // sockaddr_in
#include <stdlib.h>      // atoi()
#include <errno.h>      

#include "log.h"
#include "sqlconnRAII.h"
#include "buffer.h"
#include "httprequest.h"
#include "httpresponse.h"

class HttpConn
{
private:
    int _fd;
    struct sockaddr_in _addr;
    bool _isClose;
    int _iovCnt;
    struct iovec _iov[2];

    Buffer _readBuff;
    Buffer _writeBuff;

    HttpRequest _request;
    HttpResponse _response;
public:
    HttpConn();
    ~HttpConn();

    void init(int sockFd, const sockaddr_in& addr);
    ssize_t read(int* saveErrno);
    ssize_t write(int* saveErrno);
    void closeConn();
    int getFd() const;
    int getPort() const;
    const char* getIP() const;
    sockaddr_in getAddr() const;
    bool process();
    int toWriteBytes() { 
        return _iov[0].iov_len + _iov[1].iov_len; 
    }
    bool isKeepAlive() const {
        return _request.isKeepAlive();
    }

    static bool isET;
    static const char* srcDir;
    static std::atomic<int> userCount;
};
