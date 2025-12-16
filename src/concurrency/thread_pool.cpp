#include "cppkit/concurrency/thread_pool.hpp"

namespace cppkit::concurrency
{
    ThreadPool::ThreadPool(size_t threadCount)
    {
        if (threadCount == 0)
            threadCount = 1;
        stop.store(false, std::memory_order_release);
        for (size_t i = 0; i < threadCount; ++i)
        {
            workers.emplace_back(
                [this]
                {
                    for (;;)
                    {
                        std::function<void()> task;
                        {
                            std::unique_lock<std::mutex> lock(this->mtx);
                            this->cv.wait(
                                lock,
                                [this] { return this->stop.load(std::memory_order_acquire) || !this->tasks.empty(); });
                            if (this->stop.load(std::memory_order_acquire) && this->tasks.empty())
                                return;
                            task = std::move(this->tasks.front());
                            this->tasks.pop();
                        }
                        task();
                    }
                });
        }
    }

    ThreadPool::~ThreadPool()
    {
        stop.store(true, std::memory_order_release);
        cv.notify_all();
        for (std::thread& worker : workers)
        {
            if (worker.joinable())
                worker.join();
        }
    }

    size_t ThreadPool::workerCount() const noexcept
    {
        return workers.size();
    }

    void ThreadPool::shutdown()
    {
        stop.store(true, std::memory_order_release);
        cv.notify_all();
        for (std::thread& worker : workers)
        {
            if (worker.joinable())
                worker.join();
        }
        workers.clear();
    }

    void ThreadPool::shutdownNow()
    {
        stop.store(true, std::memory_order_release);
        {
            std::lock_guard lock(mtx);
            while (!tasks.empty())
                tasks.pop();
        }
        this->shutdown();
    }
} // namespace cppkit::concurrency
