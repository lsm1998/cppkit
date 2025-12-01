#include "cppkit/concurrency/thread_group.hpp"

namespace cppkit::concurrency
{
  void ThreadGroup::run(std::function<void()>&& task)
  {
    {
      std::thread worker([fn = std::move(task), this]() mutable
      {
        try
        {
          fn();
        }
        catch (...)
        {
          // 存储异常信息
          std::lock_guard lk(_mtx);
          _exceptions.push_back(std::current_exception());
        }
      });
      std::lock_guard lk(_mtx);
      _threads.emplace_back(std::move(worker));
    }
  }

  void ThreadGroup::wait()
  {
    std::vector<std::thread> pending;
    {
      std::lock_guard lk(_mtx);
      pending.swap(_threads);
    }

    for (auto& t : pending)
      if (t.joinable())
        t.join();
  }

  std::vector<std::exception_ptr> ThreadGroup::getExceptions() const noexcept
  {
    std::lock_guard lk(_mtx);
    return _exceptions;
  }
}