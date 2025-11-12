#include "cppkit/concurrency/wait_group.hpp"
#include "cppkit/random.hpp"
#include <iostream>
#include <thread>

void worker(cppkit::concurrency::WaitGroup& wg)
{
  std::this_thread::sleep_for(std::chrono::milliseconds(cppkit::Random::nextInt(2000)));
  wg.done();
  std::cout << "worker done" << std::endl;
}

int main()
{
  for (int i = 0; i < 100; ++i)
  {
    std::cout << "start" << std::endl;
    cppkit::concurrency::WaitGroup wg;
    wg.add(2);
    // 启动两个工作线程
    std::thread t1(worker, std::ref(wg));
    std::thread t2(worker, std::ref(wg));
    t1.join();
    t2.join();
    std::cout << "waiting for workers to finish..." << std::endl;
    wg.wait();
    std::cout << "end" << std::endl;
  }
  return 0;
}