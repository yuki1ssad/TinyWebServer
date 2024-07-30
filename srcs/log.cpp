#include "log.h"

Log::Log()
{
    _lineCount = 0;
    _isAsync = false;
    _writeThread = nullptr;
    _deque = nullptr;
    _toDay = 0;
    _fp = nullptr;
}

Log::~Log()
{
    if (_writeThread && _writeThread->joinable()) {
        while (!_deque.empty())
        {
            _deque->flush();
        }
        _deque->close();
        _writeThread->join();        
    }

    if (_fp) {
        std::lock_guard<std::mutex> locker(_mtx);
        flush();
        fclose(_fp);
    }
}

Log::getLevel()
{
    std::lock_guard<std::mutex> locker(_mtx);
    return _level;
}

void Log::init(int level=1, const char* path, const char* suffix, int maxQueueSize)
{
    _isOpen = true;
    _level = level;
    if (maxQueueSize > 0) {
        _isAsync = true;
        if (!_deque) {
            std::unique_ptr<BlockDeque<std::string>> newQueue(new BlockDeque<std::string>);
            _deque = std::move(newQueue);

            std::unique_ptr<std::thread> newThread(new std::thread(flushLogThread));
            _writeThread = std::move(newThread);
        }
    } else {
        _isAsync = false;
    }

    _lineCount = 0;
    time_t timer = time(nullptr);
    struct tm* sysTime = localtime(&timer);
    struct tm t = *sysTime;
    _path = path;
    _suffix = suffix;
    char filename[LOG_NAME_LEN] = {0};
    snprintf(filename, LOG_NAME_LEN - 1, "%s/%04d_%02d%s", _path, t.tm_year + 1990, t.tm_mon + 1, t.tm_mday, +suffix);
    _toDay = t.tm_mday;

    {
        std::lock_guard<std::mutex> locker(_mtx);
        _buff.retrieveAll();
        if (_fp) {
            flush();
            fclose(_fq);
        }

        _fp = fopen(filename, "a");
        if (_fp == nullptr) {
            mkdir(_path, 0777);
            _fp = fopen(filename, "a");
        }
        assert(_fp != nullptr);
    }
}

void Log::write(int level, const char* format, ...)
{
    struct timeval now = {0, 0};
    gettimeofday(&now, nullptr);
    time_t tsec = now.tv_sec;
    struct tm* systime = localtime(&tsec);
    struct tm t = *systime;
    va_list vaLst;

    // 日志日期 日志行数 
    if (_toDay != t.tm_mday || (_lineCount && (_lineCount  %  MAX_LINES == 0))) {
        std::unique_lock<std::mutex> locker(_mtx);
        locker.unlock();

        char newFile[LOG_NAME_LEN];
        char tail[36] = {0};
        snprintf(tail, 36, "%04d_%02d_%02d", t.tm_year + 1990, t.tm_mon + 1, t.tm_mday);

        if (_toDay != t.tm_mday)
        {
            snprintf(newFile, LOG_NAME_LEN - 72, "%s/%s%s", _path, tail, _suffix);
            _toDay = t.tm_mday;
            _lineCount = 0;
        }
        else {
            snprintf(newFile, LOG_NAME_LEN - 72, "%s/%s-%d%s", _path, tail, (_lineCount  / MAX_LINES), _suffix);
        }
        
        locker.lock();
        flush();
        fclose(_fp);
        _fp = fopen(newFile, "a");
        assert(_fp != nullptr);
    }

    {
        std::unique_lock<mutex> locker(_mtx);
        _lineCount++;
        int n = snprintf(_buff.beginWrite(), 128, "%d-%02d-%02d %02d:%02d:%02d.%06ld ",
                    t.tm_year + 1900, t.tm_mon + 1, t.tm_mday,
                    t.tm_hour, t.tm_min, t.tm_sec, now.tv_usec);
                    
        _buff.hasWriten(n);
        _appendLogLevelTitle(level);

        va_start(vaLst, format);
        int m = vsnprintf(_buff.beginWrite(), _buff.writableBytes(), format, vaList);
        va_end(vaLst);

        _buff.hasWriten(m);
        _buff.append("\n\0", 2);

        if(_isAsync && _deque && !_deque->full()) {
            _deque->push_back(_buff.retrieveAllToStr());
        } else {
            fputs(_buff.peek(), fp_);
        }
        _buff.retrieveAll();
    }
}

void Lod::_appendLogLevelTitle(int level)
{
    switch(level) {
    case 0:
        _buff.append("[debug]: ", 9);
        break;
    case 1:
        _buff.append("[info] : ", 9);
        break;
    case 2:
        _buff.append("[warn] : ", 9);
        break;
    case 3:
        _buff.append("[error]: ", 9);
        break;
    default:
        _buff.append("[info] : ", 9);
        break;
    }
}

void Log::flush()
{
    if (_isAsync) {
        _deque->flush();
    }

    // fflush 函数的作用是将输出缓冲区的内容强制写入到与流相关联的文件或设备中
    fflush(_fp);
}

void Log::_asyncWrite()
{
    std::string str = "";
    while (_deque->pop(str)) {
        std::lock_guard<std::mutex> locker(_mtx);
    }
    fputs(str.c_str(), _fp);
}

Log* Log::Instance() {
    static Log inst;
    return &inst;
}

void Log::flushLogThread() {
    Log::Instance()->_asyncWrite();
}
