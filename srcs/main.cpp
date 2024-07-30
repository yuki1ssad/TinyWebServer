#include <unistd.h>
#include "webserver.h"

int main() {
    WebServer server(
        8000, 3, 60000, false,             // 端口 ET模式 timeoutMs 优雅退出
        3306, "debian-sys-maint", "CmJ5ukcMGnE0PTJB", "mydb", // Mysql配置
        12, 6, true, 1, 1024);             // 连接池数量 线程池数量 日志开关 日志等级 日志异步队列容量
    // 电脑配置：4核8线程
    server.start();
} 