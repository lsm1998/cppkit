#include "cppkit/concurrency/thread_group.hpp"

namespace cppkit::concurrency
{
  void ThreadGroup::run(std::function<void()>&& task)
  {
    {
      std::thread worker([fn = std::move(task)]() mutable
      {
        try
        {
          fn();
        }
        catch (...)
        {
          // 防止异常导致 std::terminate；如需透传异常，可在此记录并在 wait() 中重新抛出
        }
      });
      std::lock_guard<std::mutex> lk(_mtx);
      _threads.emplace_back(std::move(worker));
    }
  }

  void ThreadGroup::wait()
  {
    std::vector<std::thread> pending;
    {
      std::lock_guard<std::mutex> lk(_mtx);
      pending.swap(_threads);
    }

    for (auto& t : pending)
      if (t.joinable())
        t.join();
  }
}