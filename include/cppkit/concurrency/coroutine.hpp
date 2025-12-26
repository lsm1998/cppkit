#pragma once

#include <coroutine>
#include <exception>
#include <iostream>
#include <optional>
#include <queue>
#include <thread>
#include <vector>
#include <atomic>
#include <functional>
#include <utility>

namespace cppkit::concurrency
{
    // Forward declarations
    template <typename T>
    class Task;

    template <>
    class Task<void>;

    // Scheduler class - fully defined first
    class Scheduler
    {
        static thread_local Scheduler* current_scheduler;

        std::queue<std::coroutine_handle<>> ready_queue_;
        std::atomic<bool> running_{false};
        std::atomic<size_t> task_count_{0};

    public:
        Scheduler() noexcept = default;
        ~Scheduler() noexcept = default;

        Scheduler(const Scheduler&) = delete;
        Scheduler& operator=(const Scheduler&) = delete;
        Scheduler(Scheduler&&) = delete;
        Scheduler& operator=(Scheduler&&) = delete;

        // Get current Scheduler for this thread
        static Scheduler* current() noexcept
        {
            return current_scheduler;
        }

        // Schedule a coroutine
        void schedule(const std::coroutine_handle<> handle) noexcept
        {
            ready_queue_.push(handle);
            ++task_count_; // Increment Task count when scheduling
        }

        // Run the Scheduler
        void run()
        {
            current_scheduler = this;
            running_ = true;

            running_ = true;

            while (running_ && task_count_ > 0)
            {
                if (ready_queue_.empty())
                {
                    // No work, yield thread
                    std::this_thread::yield();
                    continue;
                }

                // Get next coroutine to run
                auto handle = ready_queue_.front();
                ready_queue_.pop();

                // Resume it
                if (handle)
                {
                    handle.resume();
                    --task_count_; // Decrement after Task completes
                }
            }

            // Cleanup any remaining Tasks
            while (!ready_queue_.empty())
            {
                auto handle = ready_queue_.front();
                ready_queue_.pop();
                handle.destroy();
                task_count_--;
            }

            current_scheduler = nullptr;
        }

        // Stop the Scheduler
        void stop() noexcept
        {
            running_ = false;
        }
    };

    // Initialize thread-local current Scheduler
    thread_local Scheduler* Scheduler::current_scheduler = nullptr;

    // Task class for coroutines - non-void specialization
    template <typename T>
    class Task
    {
    public:
        struct promise_type
        {
            std::optional<T> result;
            std::exception_ptr exception;
            std::coroutine_handle<> continuation;

            promise_type() noexcept = default;
            ~promise_type() noexcept = default;

            Task<T> get_return_object() noexcept
            {
                return Task{std::coroutine_handle<promise_type>::from_promise(*this)};
            }

            // Initial suspend: always suspend initially
            [[nodiscard]] std::suspend_always initial_suspend() const noexcept
            {
                return {};
            }

            // Final suspend: schedule continuation if exists
            struct final_awaiter
            {
                bool await_ready() const noexcept
                {
                    return false;
                }

                std::coroutine_handle<> await_suspend(std::coroutine_handle<promise_type> handle) noexcept
                {
                    if (handle.promise().continuation)
                    {
                        // Schedule the continuation
                        Scheduler::current()->schedule(handle.promise().continuation);
                    }
                    return std::noop_coroutine();
                }

                void await_resume() noexcept
                {
                    // Do nothing
                }
            };

            final_awaiter final_suspend() const noexcept
            {
                return {};
            }

            // Return value for non-void Task
            template <std::convertible_to<T> U>
            void return_value(U&& u) noexcept(std::is_nothrow_constructible_v<T, U&&>)
            {
                result.emplace(std::forward<U>(u));
            }

            // Handle exceptions
            void unhandled_exception() noexcept
            {
                exception = std::current_exception();
            }

            // Allocator support
            static void* operator new(std::size_t size)
            {
                return ::operator new(size);
            }

            static void operator delete(void* ptr)
            {
                ::operator delete(ptr);
            }
        };

    private:
        std::coroutine_handle<promise_type> handle_;

    public:
        explicit Task(std::coroutine_handle<promise_type> handle) noexcept : handle_(handle)
        {
        }

        Task() noexcept : handle_(nullptr)
        {
        }

        Task(Task&& other) noexcept : handle_(std::exchange(other.handle_, nullptr))
        {
        }

        Task& operator=(Task&& other) noexcept
        {
            if (this != &other)
            {
                if (handle_)
                {
                    handle_.destroy();
                }
                handle_ = std::exchange(other.handle_, nullptr);
            }
            return *this;
        }

        ~Task() noexcept
        {
            if (handle_)
            {
                handle_.destroy();
            }
        }

        // Not copyable
        Task(const Task&) = delete;
        Task& operator=(const Task&) = delete;

        // Schedule this Task on the given Scheduler
        void schedule_on(Scheduler& sched) noexcept
        {
            sched.schedule(handle_);
            handle_ = nullptr; // Transfer ownership
        }

        // Awaitable interface
        bool await_ready() const noexcept
        {
            return false; // Always suspend
        }

        template <typename promise_type>
        void await_suspend(std::coroutine_handle<promise_type> caller) const noexcept
        {
            // Store the caller as continuation
            handle_.promise().continuation = caller;
            // Schedule this coroutine to run
            Scheduler::current()->schedule(handle_);
        }

        // Final awaiter for Task
        T await_resume() const
        {
            if (handle_.promise().exception)
            {
                std::rethrow_exception(handle_.promise().exception);
            }

            return std::move(*handle_.promise().result);
        }
    };

    // Task class specialization for void return type
    template <>
    class Task<void>
    {
    public:
        struct promise_type
        {
            std::exception_ptr exception;
            std::coroutine_handle<> continuation;

            promise_type() noexcept = default;
            ~promise_type() noexcept = default;

            Task<void> get_return_object() noexcept
            {
                return Task{std::coroutine_handle<promise_type>::from_promise(*this)};
            }

            // Initial suspend: always suspend initially
            std::suspend_always initial_suspend() const noexcept
            {
                return {};
            }

            // Final suspend: schedule continuation if exists
            struct final_awaiter
            {
                bool await_ready() const noexcept
                {
                    return false;
                }

                std::coroutine_handle<> await_suspend(std::coroutine_handle<promise_type> handle) noexcept
                {
                    if (handle.promise().continuation)
                    {
                        // Schedule the continuation
                        Scheduler::current()->schedule(handle.promise().continuation);
                    }
                    return std::noop_coroutine();
                }

                void await_resume() noexcept
                {
                    // Do nothing
                }
            };

            final_awaiter final_suspend() const noexcept
            {
                return {};
            }

            // Return void
            void return_void() noexcept
            {
                // No result to store
            }

            // Handle exceptions
            void unhandled_exception() noexcept
            {
                exception = std::current_exception();
            }

            // Allocator support
            void* operator new(const std::size_t size)
            {
                return ::operator new(size);
            }

            void operator delete(void* ptr)
            {
                ::operator delete(ptr);
            }
        };

    private:
        std::coroutine_handle<promise_type> handle_;

    public:
        explicit Task(std::coroutine_handle<promise_type> handle) noexcept : handle_(handle)
        {
        }

        Task() noexcept : handle_(nullptr)
        {
        }

        Task(Task&& other) noexcept : handle_(std::exchange(other.handle_, nullptr))
        {
        }

        Task& operator=(Task&& other) noexcept
        {
            if (this != &other)
            {
                if (handle_)
                {
                    handle_.destroy();
                }
                handle_ = std::exchange(other.handle_, nullptr);
            }
            return *this;
        }

        ~Task() noexcept
        {
            if (handle_)
            {
                handle_.destroy();
            }
        }

        // Not copyable
        Task(const Task&) = delete;
        Task& operator=(const Task&) = delete;

        void schedule_on(Scheduler& sched) noexcept
        {
            sched.schedule(handle_);
            handle_ = nullptr;
        }

        bool await_ready() const noexcept
        {
            return false;
        }

        template <typename promise_type>
        void await_suspend(std::coroutine_handle<promise_type> caller) const noexcept
        {
            // Store the caller as continuation
            handle_.promise().continuation = caller;
            // Schedule this coroutine to run
            Scheduler::current()->schedule(handle_);
        }

        // Final awaiter for void Task
        void await_resume() const
        {
            if (handle_.promise().exception)
            {
                std::rethrow_exception(handle_.promise().exception);
            }
        }
    };

    // A simple awaitable that yields to Scheduler
    auto yield() noexcept
    {
        struct yield_awaiter
        {
            bool await_ready() const noexcept
            {
                return false;
            }

            std::coroutine_handle<> await_suspend(std::coroutine_handle<> handle) noexcept
            {
                // Schedule the current coroutine to run again
                Scheduler::current()->schedule(handle);
                // Return empty handle to suspend
                return std::noop_coroutine();
            }

            void await_resume() noexcept
            {
                // Do nothing
            }
        };

        return yield_awaiter{};
    }

    // Mutex for coroutines
    class mutex
    {
    private:
        bool locked_ = false;
        std::queue<std::coroutine_handle<>> wait_queue_;

    public:
        mutex() noexcept = default;
        ~mutex() noexcept = default;

        mutex(const mutex&) = delete;
        mutex& operator=(const mutex&) = delete;
        mutex(mutex&&) = delete;
        mutex& operator=(mutex&&) = delete;

        // Lock the mutex (awaitable)
        auto lock() noexcept
        {
            struct lock_awaiter
            {
            private:
                mutex& mutex_;

            public:
                explicit lock_awaiter(mutex& m) noexcept : mutex_(m)
                {
                }

                bool await_ready() const noexcept
                {
                    return !mutex_.locked_;
                }

                bool await_suspend(std::coroutine_handle<> handle) noexcept
                {
                    if (mutex_.locked_)
                    {
                        mutex_.wait_queue_.push(handle);
                        return true;
                    }
                    else
                    {
                        mutex_.locked_ = true;
                        return false;
                    }
                }

                void await_resume() noexcept
                {
                    // Already locked
                }
            };

            return lock_awaiter{*this};
        }

        // Unlock the mutex
        void unlock() noexcept
        {
            if (wait_queue_.empty())
            {
                locked_ = false;
            }
            else
            {
                // Wake up the next coroutine in queue
                auto handle = wait_queue_.front();
                wait_queue_.pop();
                Scheduler::current()->schedule(handle);
            }
        }
    };

    // Condition variable for coroutines
    class condition_variable
    {
    private:
        std::queue<std::coroutine_handle<>> wait_queue_;

    public:
        condition_variable() noexcept = default;
        ~condition_variable() noexcept = default;

        condition_variable(const condition_variable&) = delete;
        condition_variable& operator=(const condition_variable&) = delete;
        condition_variable(condition_variable&&) = delete;
        condition_variable& operator=(condition_variable&&) = delete;

        // Wait on condition variable (awaitable)
        auto wait() noexcept
        {
            struct wait_awaiter
            {
            private:
                condition_variable& cv_;

            public:
                explicit wait_awaiter(condition_variable& cv) noexcept : cv_(cv)
                {
                }

                bool await_ready() const noexcept
                {
                    return false;
                }

                void await_suspend(std::coroutine_handle<> handle) noexcept
                {
                    cv_.wait_queue_.push(handle);
                }

                void await_resume() noexcept
                {
                    // Do nothing
                }
            };

            return wait_awaiter{*this};
        }

        // Notify one waiting coroutine
        void notify_one() noexcept
        {
            if (!wait_queue_.empty())
            {
                auto handle = wait_queue_.front();
                wait_queue_.pop();
                Scheduler::current()->schedule(handle);
            }
        }

        // Notify all waiting coroutines
        void notify_all() noexcept
        {
            while (!wait_queue_.empty())
            {
                auto handle = wait_queue_.front();
                wait_queue_.pop();
                Scheduler::current()->schedule(handle);
            }
        }
    };
}
