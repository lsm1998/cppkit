#include "cppkit/timer.hpp"

namespace cppkit
{
    Timer::~Timer()
    {
        stop();
    }

    void Timer::cancel(const TimerId id)
    {
        std::lock_guard lock(mutex);
        if (const auto it = timerMap.find(id); it != timerMap.end())
        {
            // 标记为取消，下次 tick 遇到时会移除
            // 这里使用 weak_ptr 防止在 callback 执行时被销毁
            if (const auto ptr = it->second.lock())
            {
                ptr->canceled = true;
            }
            timerMap.erase(it);
        }
    }

    void Timer::stop()
    {
        running = false;
        if (worker.joinable())
        {
            worker.join();
        }
    }

    void Timer::start()
    {
        running = true;
        worker = std::thread([this] { this->runLoop(); });
    }

    void Timer::runLoop()
    {
        while (running)
        {
            auto start_time = std::chrono::steady_clock::now();

            {
                std::lock_guard<std::mutex> lock(mutex);
                processCurrentSlot();
            }

            // 计算下一格时间，补偿执行耗时
            auto end_time = std::chrono::steady_clock::now();
            auto elapsed = end_time - start_time;

            if (auto sleepTime = tickDuration - elapsed; sleepTime.count() > 0)
            {
                std::this_thread::sleep_for(sleepTime);
            }

            // 指针前移
            {
                std::lock_guard<std::mutex> lock(mutex);
                currentSlot = (currentSlot + 1) % wheelSize;
            }
        }
    }

    void Timer::processCurrentSlot()
    {
        auto& slot_list = slots[currentSlot];

        auto it = slot_list.begin();
        while (it != slot_list.end())
        {
            auto& task = *it;

            // 1. 如果任务已被取消，直接删除
            if (task->canceled)
            {
                timerMap.erase(task->id); // 确保 map 也清理
                it = slot_list.erase(it);
                continue;
            }

            // 2. 检查圈数
            if (task->rounds > 0)
            {
                task->rounds--; // 还没到时间，圈数减一，跳过
                ++it;
            }
            else
            {
                try
                {
                    task->callback();
                }
                catch (...)
                {
                }

                if (task->periodic && !task->canceled)
                {
                    // 周期任务：重新计算位置并插入
                    reschedulePeriodic(task);
                    it = slot_list.erase(it); // 从当前位置移除 (已经移到新位置了)
                }
                else
                {
                    // 一次性任务：彻底移除
                    timerMap.erase(task->id);
                    it = slot_list.erase(it);
                }
            }
        }
    }

    void Timer::reschedulePeriodic(const TaskPtr& task)
    {
        // 重新计算
        size_t ticks = task->interval.count() / tickDuration.count();
        if (ticks == 0) ticks = 1;

        const size_t rounds = ticks / wheelSize;
        const size_t targetSlot = (currentSlot + ticks) % wheelSize;

        task->rounds = rounds;
        slots[targetSlot].push_back(task);
    }
}
