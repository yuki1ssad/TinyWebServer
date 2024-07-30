cmake_minimum_required(VERSION 3.10)
project(WebServer)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 添加编译选项
add_compile_options(-Wall -Wextra -O2)

# 包含头文件目录
include_directories(include)

# 资源文件处理（复制到构建目录）
file(GLOB_RECURSE RESOURCES resources/*)
file(COPY ${RESOURCES} DESTINATION ${CMAKE_BINARY_DIR}/resources)

# 指定源文件
set(SOURCE_FILES
    srcs/main.cpp
    srcs/webserver.cpp
    srcs/httpconn.cpp
    srcs/httprequest.cpp
    srcs/httpresponse.cpp
    srcs/epoller.cpp
    srcs/buffer.cpp
    srcs/log.cpp
    srcs/sqlconnpool.cpp
    srcs/timer.cpp
)

# 创建可执行文件
add_executable(${PROJECT_NAME} ${SOURCE_FILES})

# 查找 Threads 库
find_package(Threads REQUIRED)
# 链接 Threads 库
target_link_libraries(${PROJECT_NAME} ${CMAKE_THREAD_LIBS_INIT})
