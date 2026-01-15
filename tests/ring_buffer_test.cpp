#include "cppkit/concurrency/ring_buffer.hpp"
#include <iostream>
#include <vector>
#include <thread>

void print_pass(const char* name)
{
    std::cout << "[PASS] " << name << std::endl;
}

void print_info(const char* msg)
{
    std::cout << "[INFO] " << msg << std::endl;
}

void test_correctness()
{
    cppkit::concurrency::RingBuffer<int, 1024> queue;

    constexpr int NUM_PRODUCERS = 4;
    constexpr int NUM_CONSUMERS = 4;
    constexpr int ITEMS_PER_PRODUCER = 100000;
    constexpr int TOTAL_ITEMS = NUM_PRODUCERS * ITEMS_PER_PRODUCER;

    std::atomic<long long> total_consumed_sum{0};
    std::atomic total_consumed_count{0};
    std::vector<std::thread> threads;

    // 启动消费者
    threads.reserve(NUM_CONSUMERS);
    for (int i = 0; i < NUM_CONSUMERS; ++i)
    {
        threads.emplace_back([&]()
        {
            int val;
            while (true)
            {
                if (queue.pop(val))
                {
                    total_consumed_sum.fetch_add(val, std::memory_order_relaxed);
                    total_consumed_count.fetch_add(1, std::memory_order_release);
                }
                else
                {
                    std::this_thread::yield();
                }
                if (total_consumed_count.load(std::memory_order_acquire) >= TOTAL_ITEMS)
                {
                    if (queue.size() == 0)
                    {
                        break;
                    }
                }
            }
        });
    }

    for (int i = 0; i < NUM_PRODUCERS; ++i)
    {
        threads.emplace_back([&]()
        {
            for (int j = 0; j < ITEMS_PER_PRODUCER; ++j)
            {
                while (!queue.push(j))
                {
                    std::this_thread::yield();
                }
            }
        });
    }

    // 等待所有线程完成
    for (auto& t : threads)
    {
        if (t.joinable()) t.join();
    }

    long long expected = 0;
    for (int i = 0; i < NUM_PRODUCERS; i++)
    {
        constexpr long long n = ITEMS_PER_PRODUCER;
        expected += n * (n - 1) / 2;
    }

    if (total_consumed_sum == expected)
    {
        print_pass("MPMC Correctness (Data Integrity)");
    }
    else
    {
        std::cerr << "[FAIL] Data mismatch! Expected: " << expected
            << ", Actual: " << total_consumed_sum << std::endl;
        exit(1);
    }
}

int main()
{
    std::cout << "=== MPMC Ring Buffer Test Suite ===" << std::endl;

    test_correctness();

    std::cout << "=== All Tests Passed ===" << std::endl;
    return 0;
}
