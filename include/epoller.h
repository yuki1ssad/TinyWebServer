#pragma once

#include <sys/epoll.h> //epoll_ctl()
#include <fcntl.h>  // fcntl()
#include <unistd.h> // close()
#include <cassert>
#include <vector>
#include <errno.h>

class Epoller
{
private:
    int _epollFd;
    std::vector<struct epoll_event> _events;
public:
    explicit Epoller(int maxEvent=1024);
    ~Epoller();
    bool addFd(int fd, uint32_t events);
    bool modFd(int fd, uint32_t events);
    bool delFd(int fd);
    int wait(int timeoutMs = -1);
    int getEventFd(size_t i) const;
    uint32_t getEvents(size_t i) const;
};


