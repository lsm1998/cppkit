#include "cppkit/concurrency/wait_group.hpp"
#include "cppkit/random.hpp"
#include <iostream>
#include <thread>

void worker(cppkit::concurrency::WaitGroup& wg)
{
  std::this_thread::sleep_for(std::chrono::milliseconds(cppkit::Random::nextInt(2000)));
  std::cout << "worker done" << std::endl;
  wg.done();
}

int main()
{
  cppkit::concurrency::WaitGroup wg;
  for (int i = 0; i < 100; ++i)
  {
    wg.add(1);
    // 启动工作线程
    std::thread t(worker, std::ref(wg));
    t.detach();
  }

  wg.wait();
  std::cout << "end" << std::endl;
  return 0;
}