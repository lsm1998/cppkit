#include "cppkit/monitor.hpp"
#include <iostream>
#include <thread>
#include <chrono>

using namespace cppkit;

int main()
{
    Monitor monitor;

    // 测试CPU使用率（需要两次采样）
    std::cout << "\n[CPU Usage]" << std::endl;
    std::cout << "First reading (will be 0): " << monitor.GetCpuUsage() << "%" << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    const double cpu_usage = monitor.GetCpuUsage();
    std::cout << "CPU Usage: " << cpu_usage << "%" << std::endl;

    // 测试内存使用率
    std::cout << "\n[Memory Usage]" << std::endl;
    const double mem_usage = monitor.GetMemoryUsage();
    const uint64_t mem_used = monitor.GetMemoryUsedMB();
    const uint64_t mem_total = monitor.GetMemoryTotalMB();
    std::cout << "Memory Usage: " << mem_usage << "%" << std::endl;
    std::cout << "Memory Used: " << mem_used << " MB" << std::endl;
    std::cout << "Memory Total: " << mem_total << " MB" << std::endl;

    // 测试系统指标
    std::cout << "\n[System Metrics]" << std::endl;
    const SystemMetrics metrics = monitor.GetSystemMetrics();
    std::cout << "CPU Usage: " << metrics.cpu_usage_percent << "%" << std::endl;
    std::cout << "Memory Usage: " << metrics.memory_usage_percent << "%" << std::endl;
    std::cout << "Memory Used: " << metrics.memory_used_mb << " MB" << std::endl;
    std::cout << "Memory Total: " << metrics.memory_total_mb << " MB" << std::endl;

    // 测试磁盘信息
    std::cout << "\n[Disk Info]" << std::endl;
#ifdef _WIN32
    const std::string disk_path = "C:\\";
#else
    const std::string disk_path = "/";
#endif
    const DiskInfo disk = monitor.GetDiskInfo(disk_path);
    std::cout << "Path: " << disk.path << std::endl;
    std::cout << "Used: " << disk.used_mb << " MB" << std::endl;
    std::cout << "Total: " << disk.total_mb << " MB" << std::endl;
    std::cout << "Usage: " << disk.usage_percent << "%" << std::endl;

    // 测试所有磁盘信息
    std::cout << "\n[All Disks]" << std::endl;
    const std::vector<DiskInfo> disks = monitor.GetAllDiskInfo();
    std::cout << "Found " << disks.size() << " disk(s)" << std::endl;
    for (const auto& d : disks)
    {
        std::cout << "  Path: " << d.path
                  << ", Used: " << d.used_mb << " MB"
                  << ", Total: " << d.total_mb << " MB"
                  << ", Usage: " << d.usage_percent << "%" << std::endl;
    }

    // 测试系统负载平均值（仅Linux）
    std::cout << "\n[Load Average]" << std::endl;
    const std::vector<double> loadavg = monitor.GetLoadAverage();
    if (loadavg.size() >= 3)
    {
        std::cout << "1 min: " << loadavg[0] << std::endl;
        std::cout << "5 min: " << loadavg[1] << std::endl;
        std::cout << "15 min: " << loadavg[2] << std::endl;
    }
    else
    {
        std::cout << "Load average not available on this platform" << std::endl;
    }

    // 测试进程数
    std::cout << "\n[Process Count]" << std::endl;
    const int process_count = monitor.GetProcessCount();
    std::cout << "Process Count: " << process_count << std::endl;

    // 测试系统运行时间
    std::cout << "\n[System Uptime]" << std::endl;
    const uint64_t uptime = monitor.GetSystemUptime();
    std::cout << "System Uptime: " << uptime << " seconds ("
              << (uptime / 3600) << " hours "
              << ((uptime % 3600) / 60) << " minutes "
              << (uptime % 60) << " seconds)" << std::endl;

    std::cout << "\n========================================" << std::endl;
    std::cout << "Monitor Example Completed Successfully!" << std::endl;
    std::cout << "========================================" << std::endl;

    return 0;
}
