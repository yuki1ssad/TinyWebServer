# 定义编译器
CC=g++

# 定义编译选项
CFLAGS=-Iinclude -Wall -pthread -lmysqlclient -g

# 定义链接选项
LDFLAGS=-pthread -lmysqlclient

# 定义目标文件
TARGET=webserver

# 默认目标
all: $(TARGET)

# 目标依赖于源文件
$(TARGET): $(wildcard srcs/*.cpp)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

# 清理编译生成的文件
clean:
	rm -f $(TARGET)

# 伪目标，确保make命令的可读性
.PHONY: all clean