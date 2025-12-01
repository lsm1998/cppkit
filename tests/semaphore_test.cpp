#include "cppkit/concurrency/semaphore.hpp"
#include <unistd.h>
#include <string>
#include <iostream>
#include <string_view>

void printWork(const std::string_view& msg)
{
  std::cout << msg << std::endl;
}

int main()
{
  cppkit::concurrency::Semaphore sem(2);

  const auto startTime = std::chrono::steady_clock::now();

  std::vector<std::thread> threads;
  threads.reserve(5);
  for (int i = 0; i < 5; ++i)
  {
    threads.emplace_back([i, &sem]()
    {
      sem.acquire();
      sleep(2);
      printWork("Thread " + std::to_string(i) + " is working.");
      sem.release();
    });
  }
  for (auto& t : threads)
  {
    t.join();
  }
  const auto endTime = std::chrono::steady_clock::now();
  const auto duration = std::chrono::duration_cast<std::chrono::seconds>(endTime - startTime).count();
  std::cout << "Total time taken: " << duration << " seconds." << std::endl;
}