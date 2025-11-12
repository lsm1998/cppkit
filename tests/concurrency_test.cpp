#include "cppkit/concurrency/concurrency_map.hpp"
#include <iostream>

int main()
{
  cppkit::concurrency::ConcurrentHashMap<int, std::string> map;

  for (int i = 0; i < 10; ++i)
  {
    const auto key = std::to_string(i);
    map.put(i, key);
  }

  std::cout << "Map size: " << map.size() << std::endl;

  for (auto iterable = map.iterable(); auto& [key, value] : iterable)
  {
    std::cout << key << " -> " << value << std::endl;
  }
  return 0;
}