#include "cppkit/concurrency/concurrency_map.hpp"
#include "cppkit/random.hpp"
#include <iostream>
#include <thread>
#include <unistd.h>

void readWorker(cppkit::concurrency::ConcurrentHashMap<int, std::string>& map)
{
  int key = cppkit::Random::nextInt(5);

  auto item = map.get(key);
  if (item.has_value())
  {
    std::cout << key << " -> " << item.value() << std::endl;
  }
}

void writeWorker(cppkit::concurrency::ConcurrentHashMap<int, std::string>& map)
{
  int key = cppkit::Random::nextInt(5);
  std::string value = std::to_string(key);
  map.put(key, value);
}

int main()
{
  cppkit::concurrency::ConcurrentHashMap<int, std::string> map;

  int size = 10;

  // 多线程并发读写
  std::thread readGroup[size];
  std::thread writeGroup[size];

  for (int i = 0; i < size; ++i)
  {
    readGroup[i] = std::thread(readWorker, std::ref(map));
    writeGroup[i] = std::thread(writeWorker, std::ref(map));
    readGroup[i].join();
    writeGroup[i].join();
    // const auto key = std::to_string(i);
    // map.put(i, key);
  }

  sleep(10);
  std::cout << "Map size: " << map.size() << std::endl;

  for (auto iterable = map.iterable(); auto& [key, value] : iterable)
  {
    std::cout << key << " -> " << value << std::endl;
  }
  return 0;
}