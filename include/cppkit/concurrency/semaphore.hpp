#pragma once

#include <condition_variable>
#include <mutex>
#include <vector>
#include <thread>

namespace cppkit::concurrency
{
  class Semaphore
  {
  public:
    explicit Semaphore(const int initial_count = 0) : _count(initial_count)
    {
    }

    void acquire();

    void release();

    bool tryAcquire();

    bool tryAcquireFor(std::chrono::milliseconds timeout);

    bool tryAcquireUntil(const std::chrono::steady_clock::time_point& deadline);

  private:
    int _count;
    std::mutex _mtx;
    std::condition_variable _cv;
  };
} // namespace cppkit::concurrency