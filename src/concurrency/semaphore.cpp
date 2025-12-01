#include "cppkit/concurrency/semaphore.hpp"

namespace cppkit::concurrency
{
  void Semaphore::acquire()
  {
    std::unique_lock lock(_mtx);
    _cv.wait(lock, [&] { return _count > 0; });
    --_count;
  }

  void Semaphore::release()
  {
    std::unique_lock lock(_mtx);
    ++_count;
    _cv.notify_one();
  }

  bool Semaphore::tryAcquire()
  {
    std::unique_lock lock(_mtx);

    if (_count > 0)
    {
      --_count;
      return true;
    }
    return false;
  }

  bool Semaphore::tryAcquireFor(const std::chrono::milliseconds timeout)
  {
    std::unique_lock lock(_mtx);
    if (!_cv.wait_for(lock, timeout, [&] { return _count > 0; }))
      return false;
    --_count;
    return true;
  }

  bool Semaphore::tryAcquireUntil(const std::chrono::steady_clock::time_point& deadline)
  {
    std::unique_lock lock(_mtx);
    if (!_cv.wait_until(lock, deadline, [&] { return _count > 0; }))
      return false;
    --_count;
    return true;
  }
} // namespace cppkit::concurrency