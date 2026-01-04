#pragma once

#include <atomic>
#include <optional>
#include <thread>

namespace cppkit::concurrency
{
    template <typename T, size_t Capacity>
    class RingBuffer
    {
        // 保证容量是2的幂次方
        static_assert(Capacity != 0 && (Capacity & Capacity - 1) == 0, "Capacity must be power of 2");

    public:
        RingBuffer()
        {
            for (size_t i = 0; i < Capacity; ++i)
            {
                _buffer[i].sequence.store(i, std::memory_order_relaxed);
            }
        }

        ~RingBuffer()
        {
            T temp;
            while (pop(temp));
        }

        RingBuffer(const RingBuffer&) = delete;

        RingBuffer& operator=(const RingBuffer&) = delete;

        bool push(const T& data)
        {
            size_t pos = _enqueue_pos.load(std::memory_order_relaxed);

            for (;;)
            {
                Cell* cell = &_buffer[pos & _mask];
                const size_t seq = cell->sequence.load(std::memory_order_acquire);

                const intptr_t diff = static_cast<intptr_t>(seq) - static_cast<intptr_t>(pos);

                if (diff == 0)
                {
                    if (_enqueue_pos.compare_exchange_weak(pos, pos + 1, std::memory_order_relaxed))
                    {
                        new(&cell->data) T(data);

                        cell->sequence.store(pos + 1, std::memory_order_release);
                        return true;
                    }
                }
                else if (diff < 0)
                {
                    return false;
                }
                else
                {
                    pos = _enqueue_pos.load(std::memory_order_relaxed);
                }
            }
        }

        bool push(T&& data)
        {
            size_t pos = _enqueue_pos.load(std::memory_order_relaxed);

            for (;;)
            {
                Cell* cell = &_buffer[pos & _mask];
                const size_t seq = cell->sequence.load(std::memory_order_acquire);
                intptr_t diff = static_cast<intptr_t>(seq) - static_cast<intptr_t>(pos);

                if (diff == 0)
                {
                    if (_enqueue_pos.compare_exchange_weak(pos, pos + 1, std::memory_order_relaxed))
                    {
                        new(&cell->data) T(std::move(data));
                        cell->sequence.store(pos + 1, std::memory_order_release);
                        return true;
                    }
                }
                else if (diff < 0)
                {
                    return false;
                }
                else
                {
                    pos = _enqueue_pos.load(std::memory_order_relaxed);
                }
            }
        }

        bool pop(T& data)
        {
            size_t pos = _dequeue_pos.load(std::memory_order_relaxed);

            for (;;)
            {
                Cell* cell = &_buffer[pos & _mask];
                const size_t seq = cell->sequence.load(std::memory_order_acquire);

                intptr_t diff = static_cast<intptr_t>(seq) - static_cast<intptr_t>(pos + 1);

                // diff == 0 表示序列号是 pos + 1，即数据已写入完毕
                if (diff == 0)
                {
                    // 尝试抢占读取权 (CAS)
                    if (_dequeue_pos.compare_exchange_weak(pos, pos + 1, std::memory_order_relaxed))
                    {
                        // 读取数据
                        T* item_ptr = reinterpret_cast<T*>(&cell->data);
                        data = std::move(*item_ptr);
                        item_ptr->~T(); // 显式析构
                        cell->sequence.store(pos + _mask + 1, std::memory_order_release);
                        return true;
                    }
                }
                else if (diff < 0)
                {
                    return false;
                }
                else
                {
                    pos = _dequeue_pos.load(std::memory_order_relaxed);
                }
            }
        }

        std::optional<T> pop()
        {
            if (T val; pop(val))
            {
                return val;
            }
            return std::nullopt;
        }

        int size() const
        {
            const size_t deq = _dequeue_pos.load(std::memory_order_acquire);
            const size_t enq = _enqueue_pos.load(std::memory_order_acquire);
            return static_cast<int>(enq - deq);
        }

        int capacity() const
        {
            return static_cast<int>(Capacity);
        }

    private:
        struct Cell
        {
            // 序列号，用于协调读写
            std::atomic<size_t> sequence;
            // 原始内存存储数据
            alignas(T) unsigned char data[sizeof(T)];
        };

        static constexpr size_t _mask = Capacity - 1;

        // 单元格数组
        Cell _buffer[Capacity];

        // 生产者和消费者的位置指针，使用 alignas 避免伪共享
        alignas(64) std::atomic<size_t> _enqueue_pos{0};
        alignas(64) std::atomic<size_t> _dequeue_pos{0};
    };
}
