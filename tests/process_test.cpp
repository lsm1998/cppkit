#include "cppkit/process.hpp"
#include "cppkit/sync_stream.hpp"
#include "cppkit/time.hpp"
#include <iostream>

int main()
{
    // 创建管理器实例
    cppkit::ProcessManager pm;

    pm.spawn("ping", {"-c", "3", "127.0.0.1"}, [](pid_t pid, const int code)
    {
        if (code == 0)
            cppkit::sync_stream(std::cout) << "ping命令执行完成\n";
        else
            cppkit::sync_stream(std::cout) << "ping执行失败 " << code << ".\n";

    });

    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    pm.spawn("ls", {"external"});

    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // 测试杀死进程
    const pid_t sleepPid = pm.spawn("sleep", {"10"}, [](pid_t pid, const int code)
    {
        // 被信号杀死的进程，code 通常是负数 (例如 -15 SIGTERM 或 -9 SIGKILL)
        cppkit::sync_stream(std::cout) << "sleep 执行完成 code: " << code << "\n";
    });

    std::this_thread::sleep_for(std::chrono::seconds(1));
    cppkit::ProcessManager::killProcess(sleepPid, SIGTERM); // 发送 SIGTERM

    std::this_thread::sleep_for(std::chrono::seconds(1));

    // 测试长时间运行的进程
    pm.spawn("sleep", {"50"}, [](pid_t pid, int code)
    {
    });

    std::this_thread::sleep_for(std::chrono::seconds(2));
}
