#include "cppkit/concurrency/wait_group.hpp"
#include <stdexcept>

namespace cppkit::concurrency
{
  void WaitGroup::add(const int n)
  {
    if (const auto result = counter.fetch_add(n, std::memory_order_acq_rel); result < 0)
      throw std::runtime_error("WaitGroup counter underflow");
  }

  void WaitGroup::done()
  {
    const int prev = counter.fetch_sub(1, std::memory_order_acq_rel);
    if (prev < 0)
      throw std::runtime_error("WaitGroup counter underflow");

    if (prev != 1)
    {
      return;
    }
    std::unique_lock lock(mutex);
    signal.notify_all();
  }

  void WaitGroup::wait()
  {
    std::unique_lock lock(mutex);
    signal.wait(lock, [this] { return counter.load(std::memory_order_acquire) == 0; });
  }

  bool WaitGroup::wait(const int timeout)
  {
    if (timeout <= 0)
      return counter.load(std::memory_order_acquire) == 0;
    std::unique_lock lock(mutex);
    return signal.wait_for(
        lock,
        std::chrono::milliseconds(timeout),
        [this] { return counter.load(std::memory_order_acquire) == 0; });
  }
}