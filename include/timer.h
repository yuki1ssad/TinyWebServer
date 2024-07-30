#pragma once

#include <functional>
#include <chrono>
#include <vector>
#include <unordered_map>
#include <cassert>

#include "log.h"

typedef std::function<void()> TimeoutCallBack;
typedef std::chrono::high_resolution_clock Clock;
typedef std::chrono::milliseconds MS;
typedef Clock::time_point TimeStamp;

struct TimerNode
{
    int id;
    TimeStamp expires;
    TimeoutCallBack cb;
    bool operator<(const TimerNode& t) {
        return expires < t.expires;
    }
};

class HeapTimer
{
private:
    std::vector<TimerNode> _heap;
    std::unordered_map<int, size_t> _ref;   // key: TimerNode.id, value: vector.idx

private:
    void _del(size_t i);    // 删除指定位置的结点
    void _siftup(size_t i);
    bool _siftdown(size_t i, size_t n);
    void _swapNode(size_t i, size_t j);
public:
    HeapTimer() {_heap.reserve(64);}
    ~HeapTimer() {clear();}

    void adjust(int id, int timeOut);
    void add(int id, int timeOut, const TimeoutCallBack cb);
    void dowork(int id);
    void clear();
    void tick();    // 清除超时结点
    void pop();
    int getNextTick();
};


