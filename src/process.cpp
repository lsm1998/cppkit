#include "cppkit/process.hpp"
#include "cppkit/sync_stream.hpp"
#include <csignal>
#include <iostream>
#include <ranges>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

namespace cppkit
{
    ProcessManager::ProcessManager()
    {
        monitor_thread_ = std::jthread([this](const std::stop_token& st)
        {
            this->monitorLoop(st);
        });
    }

    ProcessManager::~ProcessManager()
    {
        killAll();
    }

    pid_t ProcessManager::spawn(std::string_view program, const std::vector<std::string>& args,
                                const ProcessExitCallback& cb)
    {
        std::vector<char*> cArgs;
        cArgs.push_back(const_cast<char*>(program.data()));
        for (const auto& arg : args)
        {
            cArgs.push_back(const_cast<char*>(arg.c_str()));
        }
        cArgs.push_back(nullptr);
        const pid_t pid = fork();

        if (pid < 0)
        {
            sync_stream(std::cerr) << "Fork failed!" << std::endl;
            return -1;
        }
        if (pid == 0)
        {
            // 恢复默认信号处理（因为父进程可能忽略了一些信号）
            signal(SIGCHLD, SIG_DFL);

            // 执行程序
            execvp(program.data(), cArgs.data());

            // 如果 execvp 返回，说明出错了
            perror("execvp failed");
            _exit(127); // 必须用 _exit 防止刷新父进程缓冲区
        }

        // 父进程继续执行
        {
            std::lock_guard lock(mutex_);
            callbacks_[pid] = cb;
        }
        sync_stream(std::cout) << "[ProcMgr] Spawned PID: " << pid << " (" << program << ")" << std::endl;
        return pid;
    }

    void ProcessManager::killProcess(const pid_t pid, const int sig)
    {
        if (pid <= 0) return;
        ::kill(pid, sig);
        sync_stream(std::cout) << "[ProcMgr] Sent signal " << sig << " to PID " << pid << std::endl;
    }

    void ProcessManager::killAll()
    {
        std::lock_guard lock(mutex_);
        for (const auto& pid : callbacks_ | std::views::keys)
        {
            ::kill(pid, SIGKILL);
        }
    }

    void ProcessManager::monitorLoop(const std::stop_token& st)
    {
        while (!st.stop_requested())
        {
            int status;
            // WNOHANG: 如果没有子进程退出，立即返回 0，不阻塞
            if (const pid_t pid = waitpid(-1, &status, WNOHANG); pid > 0)
            {
                // 有子进程退出
                handleExit(pid, status);
            }
            else if (pid == 0)
            {
                // 没有子进程退出，短暂休眠以节省 CPU
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }
            else
            {
                // ECHILD: 没有子进程了
                if (errno == ECHILD)
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds(200));
                }
            }
        }
    }

    void ProcessManager::handleExit(const pid_t pid, int status)
    {
        ProcessExitCallback cb = nullptr;
        {
            std::lock_guard lock(mutex_);
            if (const auto it = callbacks_.find(pid); it != callbacks_.end())
            {
                cb = it->second;
                callbacks_.erase(it);
            }
        }

        int exit_code = 0;
        if (WIFEXITED(status))
        {
            exit_code = WEXITSTATUS(status);
        }
        else if (WIFSIGNALED(status))
        {
            exit_code = -WTERMSIG(status); // 信号退出用负数表示
        }
        // 在锁外调用回调，防止死锁
        if (cb)
        {
            cb(pid, exit_code);
        }
        sync_stream(std::cout) << "[ProcMgr] Reaped PID: " << pid << " Code: " << exit_code << std::endl;
    }
}
