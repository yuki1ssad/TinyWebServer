#pragma once

#include <unordered_map>
#include <fcntl.h>       // open
#include <unistd.h>      // close
#include <sys/stat.h>    // stat
#include <sys/mman.h>    // mmap, munmap

#include "buffer.h"
#include "log.h"

class HttpResponse
{
private:
    int _code;
    bool _isKeepAlive;
    std::string _path;
    std::string _srcDir;
    char* _mmFile;
    struct stat _mmFileStat;

    static const std::unordered_map<std::string, std::string> SUFFIX_TYPE;
    static const std::unordered_map<int, std::string> CODE_STATUS;
    static const std::unordered_map<int, std::string> CODE_PATH;

private:
    void _addStateLine(Buffer &buff);
    void _addHeader(Buffer &buff);
    void _addContent(Buffer &buff);

    void _errorHtml();
    std::string _getFileType();
public:
    HttpResponse(/* args */);
    ~HttpResponse();

    void init(const std::string& srcDir, std::string& path, bool isKeepAlive = false, int code = -1);
    void makeResponse(Buffer& buff);
    void unmapFile();
    char* file();
    size_t fileLen() const;
    void errorContent(Buffer& buff, std::string message);
    int code() const { return _code; }
};

