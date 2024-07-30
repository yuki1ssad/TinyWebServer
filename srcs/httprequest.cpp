#include "httprequest.h"

static const std::unordered_set<std::string> HttpRequest::DEFAULT_HTML
{
    "/index", "/register", "/login", "/welcom", "/video", "/picture",
};

static const std::unordered_map<std::string, int> HttpRequest::DEFAULT_HTML_TAG
{
    {"/register.html", 0}, {"/login.html", 1}, 
};

static int HttpRequest::convertHex(char ch)
{
    if(ch >= 'A' && ch <= 'F') return ch -'A' + 10;
    if(ch >= 'a' && ch <= 'f') return ch -'a' + 10;
    return ch;
}

bool HttpRequest::_parseRequestLine(const std::string& line)
{
    std::regex patten("^([^ ]*) ([^ ]*) HTTP/([^ ]*)$");    // GET /index.html HTTP/1.1  第一个捕获组 ([^ ]*) 匹配请求方法（如 GET、POST）;第二个捕获组 ([^ ]*) 匹配请求的资源路径（如 /index.html）;第三个捕获组 ([^ ]*) 匹配 HTTP 版本（如 1.1）
    std::smatch subMatch;
    if (std::regex_match(line, subMatch, patten)) {
        _method = subMatch[1];
        _path = subMatch[2];
        _version = subMatch[3];
        _state = HEADERS;
        return true;
    }
    LOG_ERROR("RequestLine Error");
    return false;
}

void HttpRequest::_parseHeader(const std::string& line)
{
    std::regex patten("^([^:]*): ?(.*)$");
    std::smatch subMatch;
    if (std::regex_match(line, subMatch, patten)) {
        _header[subMatch[1]] = subMatch[2];
    } else {
        _state = BODY;
    }
}

void HttpRequest::_parseBody(const std::string& line)
{
    _body = line;
    _parsePost();
    _state = FINISH;
    LOG_DEBUG("Body:%s, len:%d", line.c_str(), line.size());
}

void HttpRequest::_parsePath()
{
    if (_path == "/") {
        _path = "/index.html";
    } else {
        for (auto& item : DEFAULT_HTML) {
            if (item == _path) {
                _path += ".html";
                break;
            }
        }
    }
}

void HttpRequest::_parsePost()
{
    if (_method == "POST" && _header["Content-Type"] == "application/x-www-form-urlencoded") {
        _parseFromUrlencoded();
        if (DEFAULT_HTML_TAG.count(_path)) {
            int tag = DEFAULT_HTML_TAG.find(_path)->second;
            LOG_DEBUG("Tag:%d", tag);
            if (tag == 0 || tag == 1) {
                bool isLogin = (tag==1);
                if(userVerify(_post["username"], _post["password"], isLogin)) {
                    _path = "/welcome.html";
                } else {
                    _path = "/error.html";
                }
            }
        }
    }
}

void HttpRequest::_parseFromUrlencoded()
{
    if (_body.size() == 0) {
        return;
    }

    std::string key, value;
    int num = 0;
    int n = _body.size();
    int i = 0, j = 0;

    for(; i < n; i++) {
        char ch = _body[i];
        switch (ch) {
            case '=':
                key = _body.substr(j, i - j);
                j = i + 1;
                break;
            case '+':
                _body[i] = ' ';
                break;
            case '%':
                num = ConverHex(_body[i + 1]) * 16 + ConverHex(_body[i + 2]);
                _body[i + 2] = num % 10 + '0';
                _body[i + 1] = num / 10 + '0';
                i += 2;
                break;
            case '&':
                value = _body.substr(j, i - j);
                j = i + 1;
                _post[key] = value;
                LOG_DEBUG("%s = %s", key.c_str(), value.c_str());
                break;
            default:
                break;
        }
    }
    assert(j <= i);
    if (_post.count(key) == 0 && j < i) {
        value = _body.substr(j, i - j);
        _post[key] = value;
    }
}

static bool HttpRequest::userVerify(const std::string& name, const std::string& pwd, bool isLogin)
{
    if(name == "" || pwd == "") { return false; }
    LOG_INFO("Verify name:%s pwd:%s", name.c_str(), pwd.c_str());
    MYSQL* sql;
    SqlConnRAII(&sql,  SqlConnPool::Instance());
    assert(sql);
    
    bool flag = false;
    unsigned int j = 0;
    char order[256] = { 0 };
    MYSQL_FIELD *fields = nullptr;
    MYSQL_RES *res = nullptr;
    
    if(!isLogin) { flag = true; }
    // 查询用户及密码
    snprintf(order, 256, "SELECT username, password FROM user WHERE username='%s' LIMIT 1", name.c_str());
    LOG_DEBUG("%s", order);

    if(mysql_query(sql, order)) { 
        mysql_free_result(res);
        return false; 
    }
    res = mysql_store_result(sql);
    j = mysql_num_fields(res);
    fields = mysql_fetch_fields(res);

    while(MYSQL_ROW row = mysql_fetch_row(res)) {
        LOG_DEBUG("MYSQL ROW: %s %s", row[0], row[1]);
        std::string password(row[1]);
        // 注册行为 且 用户名未被使用
        if(isLogin) {
            if(pwd == password) { flag = true; }
            else {
                flag = false;
                LOG_DEBUG("pwd error!");
            }
        } else { 
            flag = false; 
            LOG_DEBUG("user used!");
        }
    }
    mysql_free_result(res);

    // 注册行为 且 用户名未被使用
    if(!isLogin && flag == true) {
        LOG_DEBUG("regirster!");
        bzero(order, 256);
        snprintf(order, 256,"INSERT INTO user(username, password) VALUES('%s','%s')", name.c_str(), pwd.c_str());
        LOG_DEBUG( "%s", order);
        if(mysql_query(sql, order)) { 
            LOG_DEBUG( "Insert error!");
            flag = false; 
        }
        flag = true;
    }
    SqlConnPool::Instance()->FreeConn(sql);
    LOG_DEBUG( "UserVerify success!!");
    return flag;
}


void HttpRequest::init()
{
    _method = _path = _version = _body = "";
    _state = REQUEST_LINE;
    _header.clear();
    _post.clear();
}

bool HttpRequest::parse(Buffer& buff)
{
    const char CRLF[] = "\r\n";
    if(buff.readableBytes() <= 0) {
        return false;
    }

    while (buff.readableBytes() && _state != FINISH)
    {
        const char* lineEnd = std::search(buff.peek(), buff.beginWriteConst(), CRLF, CRLF+2);
        std::string line(buff.peek(), lineEnd);
        switch (_state)
        {
        case REQUEST_LINE:
            if (!_parseRequestLine(line)) {
                return false;
            }
            _parsePath();
            break;

        case HEADERS:
            _parseHeader(line);
            if (buff.readableBytes() <= 2) {
                _state = FINISH;
            }
            break;
        
        case BODY:
            _parseBody(line);
            break;
        
        default:
            break;
        }

        if (lineEnd == buff.beginWrite()) {
            break;
        }
        buff.retrieveUnit(lineEnd + 2);
    }
    LOG_DEBUG("[%s], [%s], [%s]", _method.c_str(), _path.c_str(), _version.c_str());
    return true;
}

std::string HttpRequest::path() const
{
    return _path;
}

std::string& HttpRequest::path()
{
    return _path;
}

std::string HttpRequest::method() const
{
    return _method
}

std::string HttpRequest::version() const
{
    return _version;
}

std::string HttpRequest::getPost(const std::string& key) const
{
    assert(key != "");
    if(_post.count(key) == 1) {
        return _post.find(key)->second;
    }
    return "";
}

std::string HttpRequest::getPost(const char* key) const
{
    assert(key != nullptr);
    if(_post.count(key) == 1) {
        return _post.find(key)->second;
    }
    return "";
}

bool HttpRequest::isKeepAlive() const
{
    if (_header.count("Connection") == 1) {
        return _header.find("Connection")->second=="keep-alive" && _version == "1.1";
    }
    return false;
}



