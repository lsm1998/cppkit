#include "cppkit/concurrency/test.hpp"
#include <string>
#include <iostream>

int main()
{
  cppkit::concurrency::ConcurrentHashMap<std::string, int> map;

  // 并发安全操作
  map.put("key1", 100);
  auto value = map.get("key1");

  // 原子操作
  map.put_if_absent("key2", 200);
  map.replace("key2", 200, 300);

  // 遍历
  map.for_each([](const std::string& key, int value)
  {
    std::cout << key << ": " << value << std::endl;
  });
  return 0;
}