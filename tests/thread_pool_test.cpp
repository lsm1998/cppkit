#include "cppkit/concurrency/thread_pool.hpp"
#include "cppkit/concurrency/sync_map.hpp"
#include "cppkit/random.hpp"
#include <iostream>

int main()
{
  cppkit::concurrency::ThreadPool pool;

  cppkit::concurrency::SyncMap<std::string, int> map;

  map.Store("hello", 10);


  for (int i = 0; i < 10; ++i)
  {
    pool.enqueue([i]()
    {
      std::this_thread::sleep_for(std::chrono::milliseconds(cppkit::Random::nextInt(2000)));
      std::cout << "task[" << i << "] done" << std::endl;
    });
  }

  pool.shutdown();
}