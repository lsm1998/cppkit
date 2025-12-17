#pragma once

#include <functional>
#include <thread>
#include <syncstream>

namespace cppkit
{
    using ProcessExitCallback = std::function<void(pid_t, int)>;

    class ProcessManager
    {
    public:
        ProcessManager();

        ~ProcessManager();

        // 启动一个新进程
        pid_t spawn(std::string_view program, const std::vector<std::string>& args,
                    const ProcessExitCallback& cb = nullptr);

        // 发送信号杀死指定进程
        static void killProcess(pid_t pid, int sig = SIGTERM);

        // 杀死所有由此管理器启动的进程
        void killAll();

    private:
        void monitorLoop(const std::stop_token& st);

        void handleExit(pid_t pid, int status);

        std::mutex mutex_;
        std::unordered_map<pid_t, ProcessExitCallback> callbacks_;
        std::jthread monitor_thread_; // C++20 自动 Join 线程
    };
}
