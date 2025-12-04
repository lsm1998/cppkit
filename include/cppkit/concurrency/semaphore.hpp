#pragma once

#include <condition_variable>
#include <mutex>

namespace cppkit::concurrency
{
  class Semaphore
  {
  public:
    explicit Semaphore(const int initial_count = 0) : _count(initial_count)
    {
    }

    // 获取一个许可，如果没有可用许可则阻塞等待
    void acquire();

    // 释放一个许可，增加可用许可数量
    void release();

    // 尝试获取一个许可，如果有可用许可则获取成功并返回 true，否则返回 false
    bool tryAcquire();

    // 尝试在指定的超时时间内获取一个许可
    bool tryAcquireFor(std::chrono::milliseconds timeout);

    // 尝试在指定的截止时间前获取一个许可
    bool tryAcquireUntil(const std::chrono::steady_clock::time_point& deadline);

  private:
    int _count; // 当前可用许可数量

    std::mutex _mtx; // 保护许可数量的互斥锁

    std::condition_variable _cv; // 用于等待和通知许可的条件变量
  };
} // namespace cppkit::concurrency