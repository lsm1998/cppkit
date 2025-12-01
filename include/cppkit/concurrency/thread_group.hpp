#pragma once

#include <functional>
#include <mutex>
#include <thread>
#include <exception>

namespace cppkit::concurrency
{
  class ThreadGroup
  {
  public:
    ThreadGroup() = default;

    // 提交一个任务到线程组
    void run(std::function<void()>&& task);

    // 等待所有线程完成执行
    void wait();

    // 获取线程中捕获的所有异常
    [[nodiscard]] std::vector<std::exception_ptr> getExceptions() const noexcept;

  private:
    mutable std::mutex _mtx; // 保护线程列表的互斥锁

    std::vector<std::thread> _threads; // 存储线程对象

    std::vector<std::exception_ptr> _exceptions; // 存储线程中捕获的异常
  };
}