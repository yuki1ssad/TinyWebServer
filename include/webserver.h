#pragma once

#include <unordered_map>
#include <fcntl.h>      // fcntl()
#include <unistd.h>     // close()
#include <cassert>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "epoller.h"
#include "httpconn.h"
#include "threadpool.h"
#include "sqlconnRAII.h"
#include "timer.h"
#include "log.h"

class WebServer
{
private:
    int _port;
    bool _openLinger;
    int _timeoutMS;
    bool _isClose;
    int _listenFd;
    char* _srcDir;

    uint32_t _listenEvent;
    uint32_t _connEvent;

    std::unique_ptr<HeapTimer> _timer;
    std::unique_ptr<ThreadPool> _threadpool;
    std::unique_ptr<Epoller> _epoller;
    std::unordered_map<int, HttpConn> _users;

    bool _initSocket();
    void _initEventMode(int trigMode);
    void _addClient(int fd, sockaddr_in addr);

    void _dealListen();
    void _dealWrite(HttpConn* client);
    void _dealRead(HttpConn* client);

    void _sendError(int fd, const char* info);
    void _extentTime(HttpConn* client);
    void _closeConn(HttpConn* client);

    void _onRead(HttpConn* client);
    void _onWrite(HttpConn* client);
    void onProcess(HttpConn* client);

    static const int MAX_FD = 65536;

    static int setFdNonblock(int fd);
    
public:
    WebServer(
        int port, int trigMode, int timeoutMS, bool optLinger,
        int sqlPort, const char* sqlUser, const char* sqlPwd,
        const char* dbname, int connPoolNum, int threadNum,
        bool openLog, int logLevel, int logQueSize
    );
    ~WebServer();

    void start();
};

