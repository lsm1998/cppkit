#pragma once

#include <vector>
#include <list>
#include <unordered_map>
#include <memory>
#include <atomic>
#include <thread>
#include <mutex>
#include <functional>
#include <chrono>
#include <iostream>

namespace cppkit
{
    using TimerId = uint64_t;

    // 默认轮子大小
    constexpr size_t DEFAULT_WHEEL_SIZE = 512;

    // 默认时间精度 (毫秒)
    constexpr size_t DEFAULT_TICK_DURATION_MS = 100;

    // 定时器配置
    struct WheelConfig
    {
        std::chrono::milliseconds tickDuration{DEFAULT_TICK_DURATION_MS};
        size_t wheel_size{DEFAULT_WHEEL_SIZE};
    };

    class Timer
    {
        // 内部任务节点
        struct TimerNode
        {
            TimerId id; // 任务ID
            std::function<void()> callback; // 任务回调
            size_t rounds; // 剩余圈数
            bool periodic; // 是否是周期性任务
            std::chrono::milliseconds interval; // 周期时间
            bool canceled{false}; // 惰性删除标记

            TimerNode(const TimerId id, std::function<void()> cb, size_t r, bool p, std::chrono::milliseconds i)
                : id(id), callback(std::move(cb)), rounds(r), periodic(p), interval(i)
            {
            }
        };

        using TaskPtr = std::shared_ptr<TimerNode>;
        using Slot = std::list<TaskPtr>;

    public:
        explicit Timer(const WheelConfig config = WheelConfig{})
            : tickDuration(config.tickDuration),
              wheelSize(config.wheel_size),
              slots(config.wheel_size),
              currentSlot(0),
              running(false),
              nextId(1)
        {
            start();
        }

        ~Timer();

        /**
         * @brief 添加一次性任务
         * @return TimerId 用于取消
         */
        template <typename Rep, typename Period>
        TimerId setTimeout(std::chrono::duration<Rep, Period> delay, std::function<void()> task)
        {
            return addTimer(delay, std::chrono::duration<Rep, Period>::zero(), std::move(task));
        }

        /**
         * @brief 添加周期性任务
         */
        template <typename Rep, typename Period>
        TimerId setInterval(std::chrono::duration<Rep, Period> interval, std::function<void()> task)
        {
            return addTimer(interval, interval, std::move(task));
        }

        /**
         * @brief 取消任务 (O(1))
         */
        void cancel(TimerId id);

        void stop();

    private:
        void start();

        template <typename Rep, typename Period>
        TimerId addTimer(std::chrono::duration<Rep, Period> delay,
                         std::chrono::duration<Rep, Period> interval,
                         std::function<void()> task)
        {
            std::lock_guard lock(mutex);

            TimerId id = nextId++;

            // 计算需要多少个 tick
            auto delay_ms = std::chrono::duration_cast<std::chrono::milliseconds>(delay);
            if (delay_ms < tickDuration) delay_ms = tickDuration;

            const size_t ticks = delay_ms.count() / tickDuration.count();

            // 计算圈数和槽位
            size_t rounds = ticks / wheelSize;
            const size_t target_slot = (currentSlot + ticks) % wheelSize;

            auto node = std::make_shared<TimerNode>(
                id,
                std::move(task),
                rounds,
                interval.count(),
                std::chrono::duration_cast<std::chrono::milliseconds>(interval)
            );

            // 放入槽位
            slots[target_slot].push_back(node);

            // 记录 ID 映射 (用于快速取消)
            timerMap[id] = node;

            return id;
        }

        void runLoop();

        void processCurrentSlot();

        void reschedulePeriodic(const TaskPtr& task);

        std::chrono::milliseconds tickDuration;
        size_t wheelSize;

        // 环形数组：每个元素是一个链表
        std::vector<Slot> slots;
        size_t currentSlot;

        std::atomic<bool> running;
        std::thread worker;
        std::mutex mutex;

        std::atomic<TimerId> nextId;
        // ID -> Node 的弱引用映射，用于 Cancel
        std::unordered_map<TimerId, std::weak_ptr<TimerNode>> timerMap;
    };
}
