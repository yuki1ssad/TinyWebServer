#include "buffer.h"


Buffer::Buffer(int initBufferSize) : _buffer(initBufferSize), _readPos(0), _writePos(0) {}

char* Buffer::beginPtr()
{
    return &*_buffer.begin();
}

const char* Buffer::beginPtr() const
{
    return &*_buffer.begin();
}

void Buffer::makeSpace(size_t len)
{
    if (writableBytes() + prependableBytes() < len) {
        _buffer.resize(_writePos + len + 1);
    } else {
        size_t readable = readableBytes();
        std::copy(beginPtr() + _readPos, beginPtr() + _writePos, beginPtr());
        _readPos = 0;
        _writePos = _readPos + readable;
        assert(readable==readableBytes());
    }
}

size_t Buffer::writableBytes() const
{
    return _buffer.size() - _writePos;
}   

size_t Buffer::readableBytes() const 
{
    return _writePos - _readPos;
}

size_t Buffer::prependableBytes() const 
{
    return _readPos;
}

const char* Buffer::peek() const
{
    return beginPtr() + _readPos;
}

void Buffer::ensureWriteable(size_t len)
{
    if (writableBytes() < len) {
        makeSpace(len);
    }
    assert(writableBytes() >= len);
}

void Buffer::hasWriten(size_t len)
{
    _writePos += len;
}

void Buffer::retrieve(size_t len)
{
    assert(len <= readableBytes());
    _readPos += len;
}

void Buffer::retrieveUntil(const char* end)
{
    assert(peek() < end);
    retrieve(end - peek());
}

void Buffer::retrieveAll()
{
    memset(&_buffer[0], 0, _buffer.size());
    _readPos = 0;
    _writePos = 0;
}

std::string Buffer::retrieveAllToStr()
{
    std::string str(peek(), readableBytes());
    retrieveAll();
    return str;
}

const char* Buffer::beginWriteConst() const
{
    return beginPtr() + _writePos;
}

char* Buffer::beginWrite()
{
    return beginPtr() + _writePos;
}

void Buffer::append(const std::string& str)
{
    append(str.data(), str.length());
}

void Buffer::append(const char* str, size_t len)
{
    assert(str);
    ensureWriteable(len);
    std::copy(str, str + len, beginWrite());
    hasWriten(len);
}

void Buffer::append(const void* data, size_t len)
{
    assert(data);
    append(static_cast<const char*>(data), len);
}

void Buffer::append(const Buffer& buff)
{
    append(buff.peek(), buff.readableBytes());
}

ssize_t Buffer::readFd(int fd, int* saveErrno)
{
    char buff[65536];
    struct iovec iov[2];
    const size_t writeable = writableBytes();

    // 分散读， 保证数据全部读完 
    iov[0].iov_base = beginPtr() + _writePos;
    iov[0].iov_len = writeable;
    iov[1].iov_base = buff;
    iov[1].iov_len = sizeof(buff);

    const ssize_t len = readv(fd, iov, 2);
    if (len < 0) {
        *saveErrno = errno;
    } else if (static_cast<size_t>(len) <= writeable) {
        _writePos += len;
    } else {
        _writePos = _buffer.size();
        append(buff, len - writeable);
    }
    return len;    
}

ssize_t Buffer::writeFd(int fd, int* saveErrno)
{
    size_t readable = readableBytes();
    ssize_t len = write(fd, peek(), readable);
    if (len < 0) {
        *saveErrno = errno;
        return len;
    }
    _readPos += len;
    return len;
}