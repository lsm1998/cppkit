#include "cppkit/testing/test.hpp"
#include "cppkit/concurrency/sync_map.hpp"
#include <iostream>
#include <thread>
#include <vector>
#include <string>
#include <atomic>
#include <random>
#include <chrono>

using namespace cppkit::testing;
using namespace cppkit::concurrency;

// 基础功能测试
TEST(SyncMapTest, BasicStoreAndLoad)
{
    SyncMap<std::string, int> map;

    map.Store("key1", 100);
    auto [val1, found1] = map.Load("key1");
    EXPECT_TRUE(found1);
    EXPECT_EQ(100, *val1);

    auto [val2, found2] = map.Load("key2");
    EXPECT_TRUE(!found2);
    EXPECT_TRUE(val2 == nullptr);
}

// Range功能测试
TEST(SyncMapTest, Range)
{
    SyncMap<std::string, int> map;
    map.Store("apple", 1);
    map.Store("banana", 2);
    map.Store("cherry", 3);

    std::vector<std::string> keys;
    map.Range([&keys](const std::string& key, const int& value)
    {
        keys.push_back(key);
        return true;
    });

    EXPECT_EQ(3, keys.size());
}

// Delete功能测试
TEST(SyncMapTest, Delete)
{
    SyncMap<std::string, int> map;
    map.Store("key1", 100);

    map.Delete("key1");

    auto [val, found] = map.Load("key1");
    EXPECT_TRUE(!found);
    EXPECT_TRUE(val == nullptr);
}

// LoadAndDelete测试
TEST(SyncMapTest, LoadAndDelete)
{
    SyncMap<std::string, int> map;
    map.Store("key1", 100);

    auto [val, found] = map.LoadAndDelete("key1");
    EXPECT_TRUE(found);
    EXPECT_EQ(100, *val);

    auto [val2, found2] = map.Load("key1");
    EXPECT_TRUE(!found2);
}

// 并发写入测试
TEST(SyncMapTest, ConcurrentWrites)
{
    SyncMap<std::string, int> map;
    const int num_threads = 4;
    const int ops_per_thread = 1000;
    std::vector<std::thread> threads;

    for (int t = 0; t < num_threads; ++t)
    {
        threads.emplace_back([&map, t, ops_per_thread]()
        {
            for (int i = 0; i < ops_per_thread; ++i)
            {
                std::string key = "key_" + std::to_string(t * ops_per_thread + i);
                map.Store(key, i);
            }
        });
    }

    for (auto& th : threads)
    {
        th.join();
    }

    // 验证所有键都存在
    int found_count = 0;
    map.Range([&found_count](const std::string& key, const int& value)
    {
        ++found_count;
        return true;
    });

    EXPECT_EQ(num_threads * ops_per_thread, found_count);
}

// 并发读写测试
TEST(SyncMapTest, ConcurrentReadsAndWrites)
{
    SyncMap<std::string, int> map;
    const int num_threads = 8;
    const int ops_per_thread = 500;
    std::vector<std::thread> threads;
    std::atomic<int> total_reads{0};
    std::atomic<int> total_writes{0};

    // 预填充一些数据
    for (int i = 0; i < 100; ++i)
    {
        map.Store("init_key_" + std::to_string(i), i);
    }

    for (int t = 0; t < num_threads; ++t)
    {
        threads.emplace_back([&map, t, ops_per_thread, &total_reads, &total_writes]()
        {
            std::mt19937 rng(t);
            std::uniform_int_distribution<int> dist(0, 99);

            for (int i = 0; i < ops_per_thread; ++i)
            {
                if (i % 3 == 0)
                {
                    // 写入
                    std::string key = "key_" + std::to_string(t) + "_" + std::to_string(i);
                    map.Store(key, i);
                    ++total_writes;
                }
                else if (i % 3 == 1)
                {
                    // 读取初始化的键
                    int key_idx = dist(rng);
                    auto [val, found] = map.Load("init_key_" + std::to_string(key_idx));
                    ++total_reads;
                    if (found)
                    {
                        EXPECT_EQ(key_idx, *val);
                    }
                }
                else
                {
                    // Range操作
                    map.Range([](const std::string& key, const int& value)
                    {
                        return true;
                    });
                }
            }
        });
    }

    for (auto& th : threads)
    {
        th.join();
    }

    std::cout << "Total reads: " << total_reads.load()
              << ", Total writes: " << total_writes.load() << std::endl;

    EXPECT_TRUE(total_reads.load() > 0);
    EXPECT_TRUE(total_writes.load() > 0);
}

// 并发删除测试
TEST(SyncMapTest, ConcurrentDeletes)
{
    SyncMap<std::string, int> map;
    const int num_keys = 1000;

    // 预填充数据
    for (int i = 0; i < num_keys; ++i)
    {
        map.Store("key_" + std::to_string(i), i);
    }

    const int num_threads = 4;
    std::vector<std::thread> threads;
    std::atomic<int> delete_count{0};

    for (int t = 0; t < num_threads; ++t)
    {
        threads.emplace_back([&map, t, num_keys, num_threads, &delete_count]()
        {
            for (int i = t; i < num_keys; i += num_threads)
            {
                map.Delete("key_" + std::to_string(i));
                ++delete_count;
            }
        });
    }

    for (auto& th : threads)
    {
        th.join();
    }

    EXPECT_EQ(num_keys, delete_count.load());

    // 验证所有键都被删除
    int found_count = 0;
    map.Range([&found_count](const std::string& key, const int& value)
    {
        ++found_count;
        return true;
    });

    EXPECT_EQ(0, found_count);
}

// 高竞争场景测试
TEST(SyncMapTest, HighContention)
{
    SyncMap<std::string, int> map;
    const int num_threads = 16;
    const int ops_per_thread = 200;
    std::vector<std::thread> threads;
    std::atomic<int> success_count{0};

    for (int t = 0; t < num_threads; ++t)
    {
        threads.emplace_back([&map, t, ops_per_thread, &success_count]()
        {
            for (int i = 0; i < ops_per_thread; ++i)
            {
                // 所有线程竞争同一个键
                map.Store("hot_key", i);
                ++success_count;

                auto [val, found] = map.Load("hot_key");
                if (found)
                {
                    // 值应该是某个线程写入的
                    EXPECT_TRUE(*val >= 0);
                    EXPECT_TRUE(*val < ops_per_thread);
                }
            }
        });
    }

    for (auto& th : threads)
    {
        th.join();
    }

    EXPECT_EQ(num_threads * ops_per_thread, success_count.load());
}

// Dirty提升测试
TEST(SyncMapTest, DirtyPromotion)
{
    SyncMap<std::string, int> map;

    // 存储一些初始数据到read
    map.Store("read_key1", 1);
    map.Store("read_key2", 2);

    // 添加新键会创建dirty
    map.Store("dirty_key1", 3);
    map.Store("dirty_key2", 4);
    map.Store("dirty_key3", 5);

    // 触发多次miss来提升dirty
    for (int i = 0; i < 10; ++i)
    {
        map.Load("dirty_key" + std::to_string(i % 3 + 1));
    }

    // Range会自动提升dirty
    map.Range([](const std::string& key, const int& value)
    {
        return true;
    });

    // 验证所有键都可以访问
    auto [v1, f1] = map.Load("read_key1");
    EXPECT_TRUE(f1);
    EXPECT_EQ(1, *v1);

    auto [v2, f2] = map.Load("dirty_key1");
    EXPECT_TRUE(f2);
    EXPECT_EQ(3, *v2);
}

// Store更新已存在键的测试
TEST(SyncMapTest, UpdateExistingKey)
{
    SyncMap<std::string, int> map;

    map.Store("key1", 100);
    map.Store("key1", 200);
    map.Store("key1", 300);

    auto [val, found] = map.Load("key1");
    EXPECT_TRUE(found);
    EXPECT_EQ(300, *val);
}

// 并发更新测试
TEST(SyncMapTest, ConcurrentUpdates)
{
    SyncMap<std::string, int> map;
    const int num_threads = 8;
    const int ops_per_thread = 100;
    std::vector<std::thread> threads;

    for (int t = 0; t < num_threads; ++t)
    {
        threads.emplace_back([&map, t, ops_per_thread]()
        {
            for (int i = 0; i < ops_per_thread; ++i)
            {
                map.Store("shared_key", t * ops_per_thread + i);
            }
        });
    }

    for (auto& th : threads)
    {
        th.join();
    }

    auto [val, found] = map.Load("shared_key");
    EXPECT_TRUE(found);
    // 最终值应该是某个线程的最后一次写入
    EXPECT_TRUE(*val >= 0);
    EXPECT_TRUE(*val < num_threads * ops_per_thread);
}

// 混合操作测试
TEST(SyncMapTest, MixedOperations)
{
    SyncMap<std::string, int> map;
    const int num_threads = 6;
    const int ops_per_thread = 100;
    std::vector<std::thread> threads;

    for (int t = 0; t < num_threads; ++t)
    {
        threads.emplace_back([&map, t, ops_per_thread]()
        {
            for (int i = 0; i < ops_per_thread; ++i)
            {
                switch (i % 4)
                {
                    case 0:
                        map.Store("op_" + std::to_string(i), i);
                        break;
                    case 1:
                        map.Load("op_" + std::to_string(i % 10));
                        break;
                    case 2:
                        map.Delete("op_" + std::to_string(i % 10));
                        break;
                    case 3:
                        map.Range([](const std::string& k, const int& v)
                        {
                            return true;
                        });
                        break;
                }
            }
        });
    }

    for (auto& th : threads)
    {
        th.join();
    }

    // 验证map仍然可用
    map.Store("final_check", 42);
    auto [val, found] = map.Load("final_check");
    EXPECT_TRUE(found);
    EXPECT_EQ(42, *val);
}

int main()
{
    std::cout << "========================================" << std::endl;
    std::cout << "SyncMap Concurrent Test Suite" << std::endl;
    std::cout << "========================================" << std::endl;

    return RunAllTests();
}
