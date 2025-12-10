#include "cppkit/concurrency/sync_map.hpp"
#include <iostream>
#include <thread>
#include <vector>
#include <string>

int main()
{
    cppkit::concurrency::SyncMap<std::string, int> map;

    // 测试 Range
    map.Store("apple", 1);
    map.Store("banana", 2);
    map.Store("cherry", 3);

    map.Range([](const std::string& key, const int& value)
    {
        std::cout << key << ": " << value << "\n";
        return true;
    });
    return 0;
}
