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

    ~ThreadPool()
    {
      stop.store(true, std::memory_order_release);
      cv.notify_all();
      for (std::thread& worker : workers)
      {
        if (worker.joinable())
          worker.join();
      }
    }

    ThreadPool(const ThreadPool&) = delete;

    ThreadPool& operator=(const ThreadPool&) = delete;

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
        tasks.emplace([taskPtr]() { (*taskPtr)(); });
      }
      cv.notify_one();
      return res;
    }

    size_t workerCount() const noexcept;

    void shutdown();

    void shutdownNow();

  private:
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;

    mutable std::mutex mtx;
    std::condition_variable cv;
    std::atomic<bool> stop{false};
  };
} // namespace cppkit::concurrency