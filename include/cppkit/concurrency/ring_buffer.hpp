#pragma once

#include <optional>

namespace cppkit::concurrency
{
    template <typename T, size_t Capacity>
    class RingBuffer
    {
    public:
        RingBuffer() = default;

        ~RingBuffer() = default;

        // 非阻塞推入
        bool push(const T& item)
        {
            const auto current_tail = _tail.load(std::memory_order_relaxed);
            const auto next_tail = (current_tail + 1) % Capacity;

            // 检查是否满了
            if (next_tail == _head.load(std::memory_order_acquire))
            {
                return false;
            }

            _buffer[current_tail] = item;
            _tail.store(next_tail, std::memory_order_release);
            return true;
        }

        // 非阻塞弹出
        std::optional<T> pop()
        {
            const auto current_head = _head.load(std::memory_order_relaxed);

            // 检查是否为空
            if (current_head == _tail.load(std::memory_order_acquire))
            {
                return std::nullopt;
            }

            T item = std::move(_buffer[current_head]);
            _head.store((current_head + 1) % Capacity, std::memory_order_release);
            return item;
        }

        // 非阻塞查看队首元素
        std::optional<T> front() const
        {
            const auto current_head = _head.load(std::memory_order_acquire);

            // 检查是否为空
            if (current_head == _tail.load(std::memory_order_acquire))
            {
                return std::nullopt;
            }

            return _buffer[current_head];
        }

        // 非阻塞查看队尾元素
        std::optional<T> back() const
        {
            const auto current_tail = _tail.load(std::memory_order_acquire);

            if (current_tail == _head.load(std::memory_order_acquire))
            {
                return std::nullopt;
            }

            const auto last_index = (current_tail + Capacity - 1) % Capacity;
            return _buffer[last_index];
        }

    private:
        // 头尾指针，使用缓存行对齐以减少伪共享
        alignas(64) std::atomic<size_t> _head{0};
        alignas(64) std::atomic<size_t> _tail{0};

        // 固定大小数组
        T _buffer[Capacity];
    };
}
