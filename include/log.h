#pragma once

#include <memory>
#include <thread>
#include <mutex>
#include <sys/time.h>

#include "unistd.h"
#include "blockdeque.h"
#include "buffer.h"

class Log
{
private:
    static const int LOG_PATH_LEN = 256;
    static const int LOG_NAME_LEN = 256;
    static const int MAX_LINES = 50000;

    const char* _path;
    const char* _suffix;
    int _maxLines;
    int _lineCount;
    int _toDay;
    bool _isOpen;
    Buffer _buff;
    int _level;
    bool _isAsync;

    FILE* _fp;
    std::unique_ptr<BlockDeque<std::string>> _deque;
    std::unique_ptr<std::thread> _writeThread;
    std::mutex _mtx;
private:
    Log();
    void _appendLogLevelTitle(int level);
    virtual ~Log();
    void _asyncWrite();
public:
    void init(int level, const char* path="./log", const char* suffix=".log", int maxQueueCapacity=1024);
    static Log* Instance();
    static void flushLogThread();
    void write(int level, const char* format, ...);
    void flush();
    int getLevel();
    void setLevel();
    bool isOpen() {return _isOpen;}
};

#define LOG_BASE(level, format, ...) \
    do {\
        Log* log = Log::Instance();\
        if (log->isOpen() && log->getLevel() <= level) {\
            log->write(level, format, ##__VA_ARGS__); \
            log->flush();\
        }\
    } while(0);

#define LOG_DEBUG(format, ...) do {LOG_BASE(0, format, ##__VA_ARGS__)} while(0);
#define LOG_INFO(format, ...) do {LOG_BASE(1, format, ##__VA_ARGS__)} while(0);
#define LOG_WARN(format, ...) do {LOG_BASE(2, format, ##__VA_ARGS__)} while(0);
#define LOG_ERROR(format, ...) do {LOG_BASE(3, format, ##__VA_ARGS__)} while(0);

