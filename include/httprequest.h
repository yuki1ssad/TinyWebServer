#pragma once

#include <unordered_map>
#include <unordered_set>
#include <string>
#include <algorithm>
#include <regex>

#include "buffer.h"
#include "log.h"
#include "sqlconnRAII.h"

enum PARSE_STATE
{
    REQUEST_LINE,
    HEADERS,
    BODY,
    FINISH,
};

enum HTTP_CODE
{
    NO_REQUEST = 0,
    GET_REQUEST,
    BAD_REQUEST,
    NO_RESOURSES,
    FORBIDDENT_REQUEST,
    FILE_REQUEST,
    INTERNAL_ERROR,
    CLOSED_CONNECTION,
};

class HttpRequest
{
private:
    PARSE_STATE _state;
    std::string _method, _path, _version, _body;
    std::unordered_map<std::string, std::string> _header;
    std::unordered_map<std::string, std::string> _post;

    static const std::unordered_set<std::string> DEFAULT_HTML;
    static const std::unordered_map<std::string, int> DEFAULT_HTML_TAG;

private:
    static int convertHex(char ch);
    bool _parseRequestLine(const std::string& line);
    void _parseHeader(const std::string& line);
    void _parseBody(const std::string& line);
    void _parsePath();
    void _parsePost();
    void _parseFromUrlencoded();
    static bool userVerify(const std::string& name, const std::string& pwd, bool isLogin);
public:
    HttpRequest() {init();}
    ~HttpRequest()=default;

    void init();
    bool parse(Buffer& buff);

    std::string path() const;
    std::string& path();
    std::string method() const;
    std::string version() const;
    std::string getPost(const std::string& key) const;
    std::string getPost(const char* key) const;

    bool isKeepAlive() const;
};


