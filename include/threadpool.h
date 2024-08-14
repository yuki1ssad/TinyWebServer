#pragma once

#include <mutex>
#include <condition_variable>
#include <queue>
#include <functional>
#include <cassert>
#include <thread>

class ThreadPool
{
private:
    struct Pool
    {
        std::mutex mtx;
        std::condition_variable cond;
        bool isClosed;
        std::queue<std::function<void()>> tasks;
    };
    std::shared_ptr<Pool> _pool;
    
public:
    explicit ThreadPool(size_t threadNum=8) : _pool(std::make_shared<Pool>())
    {
        assert(threadNum > 0);
        for (size_t i = 0; i < threadNum; ++i) {
            std::thread(
                [pool = _pool] {
                    std::unique_lock<std::mutex> locker(pool->mtx);
                    while (true)
                    {
                        if (!pool->tasks.empty()) {
                            auto task = std::move(pool->tasks.front()); // 避免不必要的复制
                            pool->tasks.pop();
                            locker.unlock();
                            task();
                            locker.lock();
                        } else if (pool->isClosed) {
                            break;
                        } else {
                            pool->cond.wait(locker);
                        }
                    }
                }
            ).detach();
        }
    }

    ThreadPool() = default; // 显式地指示编译器生成这个默认构造函数
    ThreadPool(ThreadPool&&) = default;

    ~ThreadPool()
    {
        if (static_cast<bool>(_pool)) {
            {
                std::lock_guard<std::mutex> locker(_pool->mtx);
                _pool->isClosed = true;
            }
            _pool->cond.notify_all();
        }
    }

    template <class T>
    void addTask(T&& task)
    {
        {
            std::lock_guard<std::mutex> locker(_pool->mtx);
            _pool->tasks.emplace(std::forward<T>(task));
        }
        _pool->cond.notify_one();
    }
};

