#pragma once

#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
#include <functional>
#include <memory>
#include <type_traits>
#include <utility>
#include <stdexcept>
#include <atomic>

namespace cppkit::concurrency
{
    class ThreadPool
    {
    public:
        explicit ThreadPool(size_t threadCount = std::thread::hardware_concurrency());

        ~ThreadPool();

        ThreadPool(const ThreadPool&) = delete;

        ThreadPool& operator=(const ThreadPool&) = delete;

        // 提交一个任务到线程池，返回一个 future 用于获取任务结果
        template <class F, class... Args>
        auto enqueue(F&& f, Args&&... args) -> std::future<std::invoke_result_t<std::decay_t<F>, std::decay_t<Args>...>>
        {
            using return_type = std::invoke_result_t<std::decay_t<F>, std::decay_t<Args>...>;

            auto taskPtr = std::make_shared<std::packaged_task<return_type()>>(
                std::bind(std::forward<F>(f), std::forward<Args>(args)...));

            std::future<return_type> res = taskPtr->get_future();

            {
                std::lock_guard lock(mtx);
                if (stop.load(std::memory_order_acquire))
                    throw std::runtime_error("enqueue on stopped ThreadPool");
                tasks.emplace([taskPtr] { (*taskPtr)(); });
            }
            cv.notify_one();
            return res;
        }

        // 获取工作线程数量
        size_t workerCount() const noexcept;

        // 优雅停止线程池，等待所有任务完成后退出
        void shutdown();

        // 立即停止线程池，丢弃所有未完成的任务
        void shutdownNow();

    private:
        std::vector<std::thread> workers; // 工作线程队列

        std::queue<std::function<void()>> tasks; // 任务队列

        mutable std::mutex mtx; // 互斥锁

        std::condition_variable cv; // 条件变量

        std::atomic<bool> stop{false}; // 停止标志
    };
} // namespace cppkit::concurrency
