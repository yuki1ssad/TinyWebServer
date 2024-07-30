#include "timer.h"

void HeapTimer::_del(size_t i)
{
    assert(!_heap.empty() && i >= 0 && i < _heap.size());

    // 将要删除的结点换到队尾，然后调整堆
    size_t idx = i;
    size_t last = _heap.szie() - 1;
    assert(idx <= last);
    if (idx < last) {
        _swapNode(idx, last);
        if (!_siftdown(idx, last)) {
            _siftup(idx);
        }
    }

    // 队尾元素删除
    _ref.erase(_heap.back().id);
    _heap.pop_back();
}

void HeapTimer::_siftup(size_t i)
{
    assert(i >= 0 && i < _heap.size());
    size_t j = (i - 1) / 2;
    while (j >= 0)
    {
        if (_heap[j] < _heap[i]) {
            break;
        }
        _swapNode(i, j);
        i = j;
        j = (j - 1) / 2;
    }
}

void HeapTimer::_siftdown(size_t i, size_t n)
{
    assert(i >= 0 && i < _heap.size());
    assert(n >= 0 && n <= _heap.size());

    size_t idx = i;
    size_t j = idx * 2 + 1;
    while (j < n)
    {
        if (j + 1 < n && _heap[j + 1] < _heap[j]) {
            j++;
        }
        if (_heap[idx] < _heap[j]) {
            break;
        }
        _swapNode(idx, j);
        idx = j;
        j = idx * 2 + 1;
    }
    return idx > i;
}

void HeapTimer::_swapNode(size_t i, size_t j)
{
    assert(i >= 0 && i < _heap.size());
    assert(j >= 0 && j < _heap.size());
    std::swap(_heap[i], _heap[j]);
    _ref[_heap[i].id] = i;
    _ref[_heap[j].id] = j;
}

void HeapTimer::adjust(int id, int timeOut)
{
    assert(!_heap.empty() && _ref.count(id) > 0);
    _heap[_ref[id]].expires = Clock::now() + MS(timeOut);
    _siftdown(_ref[id], _heap.size());
}

void HeapTimer::add(int id, int timeOut, const TimeoutCallBack cb)
{
    assert(id >= 0);
    size_t i;
    if (_ref.count(id) == 0) {  // 新节点：堆尾插入，调整堆
        i = _heap.size();
        _ref[id] = i;
        _heap.push_back({id, Clock::now() + MS(timeOut), cb});
        _siftup(i);
    } else {    // 已有结点：调整堆
        i = _ref[id];
        _heap[i].expires = Clock::now() + MS(timeOut);
        _heap[i].cb = cb;
        if (!_siftdown(i, _heap.size())) {
            _siftup(i);
        }
    }
}

void HeapTimer::dowork(int id)
{
    // 删除指定id结点，并触发回调函数
    if(_heap.empty() || _ref.count(id) == 0) {
        return;
    }
    size_t i = _ref[id];
    TimerNode nd = _heap[i];
    nd.cb();
    _del(i);
}

void HeapTimer::clear()
{
    _ref.clear();
    _heap.clear();
}

void HeapTimer::tick()
{
    if (_heap.empty()) {
        return;
    }

    while (!_heap.empty())
    {
        TimerNode nd = _heap.front();
        if (std::chrono::duration_cast<MS>(nd.expires - Clock::now()).count > 0) {
            break;
        }
        nd.cb();
        pop();
    }    
}

void HeapTimer::pop()
{
    assert(!_heap.empty());
    _del(0);
}

int HeapTimer::getNextTick()
{
    tick();
    size_t res = -1;
    if (!_heap.empty()) {
        res = std::chrono::duration_cast<MS>(_heap.front().expires - Clock::now()).count;
        if (res < 0) {
            res = 0;
        }
    }
    return res;
}