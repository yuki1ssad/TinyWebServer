#pragma once

#include <vector>
#include <string>
#include <cassert>
#include <cstring>  // memset
#include <atomic>
#include <unistd.h>  // write
#include <sys/uio.h> //readv    iovec

class Buffer
{
private:
    std::vector<char> _buffer;
    std::atomic<std::size_t> _readPos;
    std::atomic<std::size_t> _writePos;

private:
    char* beginPtr();
    const char* beginPtr() const;
    void makeSpace(std::size_t len);

public:
    Buffer(int initBufferSize=1024);
    ~Buffer()=default;

    std::size_t writableBytes() const;       
    std::size_t readableBytes() const ;
    std::size_t prependableBytes() const;

    const char* peek() const;
    void ensureWriteable(std::size_t len);
    void hasWriten(std::size_t len);
    void retrieve(std::size_t len);
    void retrieveUnit(const char* end);
    void retrieveAll();
    std::string retrieveAllToStr();

    const char* beginWriteConst() const;
    char* beginWrite();
    void append(const std::string& str);
    void append(const char* str, std::size_t len);
    void append(const void* data, std::size_t len);
    void append(const Buffer& buff);

    ssize_t readFd(int fd, int* saveErrno);
    ssize_t writeFd(int fd, int* saveErrno);
};

