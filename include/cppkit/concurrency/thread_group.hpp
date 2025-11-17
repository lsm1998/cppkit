#pragma once

#include <functional>
#include <mutex>
#include <thread>

namespace cppkit::concurrency
{
  class ThreadGroup
  {
  public:
    ThreadGroup() = default;

    void run(std::function<void()>&& task);

    void wait();

  private:
    std::mutex _mtx;
    std::vector<std::thread> _threads;
  };
}