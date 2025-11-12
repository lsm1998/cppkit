#pragma once

#include <atomic>
#include <mutex>

namespace cppkit::concurrency
{
  class WaitGroup
  {
  public:
    WaitGroup() = default;

    void add(int n=1);

    void done();

    void wait();

    bool wait(int timeout);

  private:
    std::atomic_int counter{0};

    std::mutex mutex;

    std::condition_variable signal;
  };
}