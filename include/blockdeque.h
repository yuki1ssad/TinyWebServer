#pragma once

#include <deque>
#include <mutex>
#include <condition_variable>
#include <cassert>

template <class T>
class BlockDeque
{
private:
    std::deque<T> _deq;
    size_t _capacity;
    bool _isClose;
    std::mutex _mtx;
    std::condition_variable _condC;     // condConsumer
    std::condition_variable _condP;     // condProducer

public:
    explicit BlockDeque(size_t MaxCapacity=1000);
    ~BlockDeque();
    
    size_t size();
    size_t capacity();

    T front();
    T back();

    bool empty();
    bool full();

    void clear();
    void close();
    void push_back(const T& item);
    void push_front(const T& item);
    bool pop(T& item);
    bool pop(T& item, int timeout);
    void flush();
};

template <class T>
BlockDeque<T>::BlockDeque(size_t MaxCapacity) :
    _capacity(MaxCapacity)
{
    assert(MaxCapacity > 0 && "MaxCapacity should be a positive number!");
    _isClose = false;
}

template <class T>
BlockDeque<T>::~BlockDeque()
{
    close();
}

template <class T>
size_t BlockDeque<T>::size()
{
    std::lock_guard<std::mutex> locker(_mtx);
    return _deq.size();
}

template<class T>
size_t BlockDeque<T>::capacity()
{
    std::lock_guard<std::mutex> locker(_mtx);
    return _capacity;
}

template<class T>
T BlockDeque<T>::front()
{
    std::lock_guard<std::mutex> locker(_mtx);
    return _deq.front();
}

template<class T>
T BlockDeque<T>::back()
{
    std::lock_guard<std::mutex> locker(_mtx);
    return _deq.back();
}


template<class T>
bool BlockDeque<T>::empty()
{
    std::lock_guard<std::mutex> locker(_mtx);
    return _deq.empty();
}

template<class T>
bool BlockDeque<T>::full()
{
    std::lock_guard<std::mutex> locker(_mtx);
    return _deq.size() >= _capacity;
}

template<class T>
void BlockDeque<T>::clear()
{
    std::lock_guard<std::mutex> locker(_mtx);
    _deq.clear();
}

template<class T>
void BlockDeque<T>::close()
{
    {
        std::lock_guard<std::mutex> locker(_mtx);
        _deq.clear();
        _isClose = true;
    }
    _condC.notify_all();
    _condP.notify_all();    
}

template<class T>
void BlockDeque<T>::push_back(const T& item)
{
    std::unique_lock<std::mutex> locker(_mtx);
    while (_deq.size() >= _capacity) {
        _condP.wait(locker);
    }
    _deq.push_front(item);
    _condC.notify_one();
}

template<class T>
void BlockDeque<T>::push_front(const T& item)
{
    std::unique_lock<std::mutex> locker(_mtx);
    while (_deq.size() >= _capacity) {
        _condP.wait(locker);
    }
    _deq.push_back(item);
    _condC.notify_one();
}

template<class T>
bool BlockDeque<T>::pop(T& item)
{
    std::unique_lock<std::mutex> locker(_mtx);
    while (_deq.empty())
    {
        _condC.wait(locker);
        if (_isClose) {
            return false;
        }
    }
    item = _deq.front();
    _deq.pop_front();
    _condP.notify_one();
    return true;
}

template<class T>
bool BlockDeque<T>::pop(T& item, int timeout)
{
    std::unique_lock<std::mutex> locker(_mtx);
    while (_deq.empty())
    {
        if (_condC.wait_for(locker, std::chrono::seconds(timeout)) == std::cv_status::timeout) {
            return false;
        }
        if (_isClose) {
            return false;
        }
    }
    item = _deq.front();
    _deq.pop_front();
    _condP.notify_one();
    return true;    
}

template<class T>
void BlockDeque<T>::flush()
{
    _condC.notify_one();
}

