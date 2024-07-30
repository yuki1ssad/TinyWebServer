#include "httpresponse.h"


static const std::unordered_map<std::string, std::string> HttpResponse::SUFFIX_TYPE
{
    { ".html",  "text/html" },
    { ".xml",   "text/xml" },
    { ".xhtml", "application/xhtml+xml" },
    { ".txt",   "text/plain" },
    { ".rtf",   "application/rtf" },
    { ".pdf",   "application/pdf" },
    { ".word",  "application/nsword" },
    { ".png",   "image/png" },
    { ".gif",   "image/gif" },
    { ".jpg",   "image/jpeg" },
    { ".jpeg",  "image/jpeg" },
    { ".au",    "audio/basic" },
    { ".mpeg",  "video/mpeg" },
    { ".mpg",   "video/mpeg" },
    { ".avi",   "video/x-msvideo" },
    { ".gz",    "application/x-gzip" },
    { ".tar",   "application/x-tar" },
    { ".css",   "text/css "},
    { ".js",    "text/javascript "},
};
static const std::unordered_map<int, std::string> HttpResponse::CODE_STATUS
{
    {200, "OK"},
    {400, "Bad Request"},
    {403, "Forbidden"},
    {404, "Not Found"},
};
static const std::unordered_map<int, std::string> HttpResponse::CODE_PATH
{
    {400, "/400.html"},
    {403, "/403.html"},
    {404, "/404.html"},
};

void HttpResponse::_addStateLine(Buffer &buff)
{
    std::string status;
    if (CODE_STATUS.count(_code) == 1) {
        status = CODE_STATUS.find(_code)->second;
    } else {
        _code = 400;
        status = CODE_STATUS.find(400)->second;
    }
    buff.append("HTTP/1.1 " + to_string(_code) + " " + status + "\r\n");
}

void HttpResponse::_addHeader(Buffer &buff)
{
    buff.append("Connection: ");
    if(_isKeepAlive) {
        buff.append("keep-alive\r\n");
        buff.append("keep-alive: max=6, timeout=120\r\n");
    } else{
        buff.append("close\r\n");
    }
    buff.append("Content-type: " + _getFileType() + "\r\n");
}

void HttpResponse::_addContent(Buffer &buff)
{
    int srcFd = open((_srcDir + _path).data(), O_RDONLY);
    if(srcFd < 0) { 
        errorContent(buff, "File NotFound!");
        return; 
    }

    /* 将文件映射到内存提高文件的访问速度 
        MAP_PRIVATE 建立一个写入时拷贝的私有映射*/
    LOG_DEBUG("file path %s", (srcDir_ + path_).data());
    int* mmRet = (int*)mmap(0, _mmFileStat.st_size, PROT_READ, MAP_PRIVATE, srcFd, 0);
    if(*mmRet == -1) {
        errorContent(buff, "File NotFound!");
        return; 
    }
    _mmFile = (char*)mmRet;
    close(srcFd);
    buff.append("Content-length: " + to_string(mmFileStat_.st_size) + "\r\n\r\n");
}

void HttpResponse::_errorHtml()
{
    if (CODE_PATH.count(_code) == 1) {
        _path = CODE_PATH.find(_code)->second;
        stat((_srcDir + _path).data(), &_mmFileStat);
    }
}

std::string HttpResponse::_getFileType()
{
    std::string::size_type idx = _path.find_last_of('.');
    if (idx == std::string::npos) {
        return "text/plain";
    }
    std::string suffix = _path.substr(idx);
    if (SUFFIX_TYPE.count(suffix) == 1) {
        return SUFFIX_TYPE.find(suffix)->second;
    }
    return "text/plain";
}

HttpResponse::HttpResponse()
{
    _code = -1;
    _path = _srcDir = "";
    _isKeepAlive = false;
    _mmFile = nullptr;
    _mmFileStat = {0};
}

HttpResponse::~HttpResponse()
{
    unmapFile();
}

void HttpResponse::init(const std::string& srcDir, std::string& path, bool isKeepAlive = false, int code = -1)
{
    assert(_srcDir != "");
    if (_mmFile) {
        unmapFile();
    }
    _code = code;
    _isKeepAlive = isKeepAlive;
    _path = path;
    _srcDir = srcDir;
    _mmFile = nullptr;
    _mmFileStat = {0};
}

void HttpResponse::makeResponse(Buffer& buff)
{
    // 判断请求的资源文件
    if (stat((_srcDir + _path).data(), &_mmFileStat) < 0 || S_ISDIR(_mmFileStat.st_mode)) {
        _code = 404;
    } else if (!(_mmFileStat.st_mode & S_IROTH)) {  // 文件没有其他用户的读权限
        _code = 403;
    } else if (_code == -1) {
        _code = 200;
    }
    _errorHtml();
    _addStateLine(buff);
    _addHeader(buff);
    _addContent(buff);
}

void HttpResponse::unmapFile()
{
    if (_mmFile) {
        munmap(_mmFile, _mmFileStat.st_size);
        _mmFile == nullptr;
    }
}

char* HttpResponse::file()
{
    return _mmFile;
}

size_t HttpResponse::fileLen() const
{
    return _mmFileStat.st_size;
}

void HttpResponse::errorContent(Buffer& buff, std::string message)
{
    std::string body;
    std::string status;
    body += "<html><title>Error</title>";
    body += "<body bgcolor=\"ffffff\">";
    if(CODE_STATUS.count(_code) == 1) {
        status = CODE_STATUS.find(_code)->second;
    } else {
        status = "Bad Request";
    }
    body += to_string(_code) + " : " + status  + "\n";
    body += "<p>" + message + "</p>";
    body += "<hr><em>TinyWebServer</em></body></html>";

    buff.append("Content-length: " + to_string(body.size()) + "\r\n\r\n");
    buff.append(body);
}
