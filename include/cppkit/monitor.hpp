#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace cppkit
{
    // 系统监控指标结构体
    struct SystemMetrics
    {
        double cpu_usage_percent;        // CPU使用率 (0.0-100.0)
        double memory_usage_percent;     // 内存使用率 (0.0-100.0)
        uint64_t memory_used_mb;         // 已使用内存 (MB)
        uint64_t memory_total_mb;        // 总内存 (MB)
    };

    // 磁盘信息结构体
    struct DiskInfo
    {
        std::string path;                // 磁盘路径
        uint64_t used_mb;                // 已使用空间 (MB)
        uint64_t total_mb;               // 总空间 (MB)
        double usage_percent;            // 使用率 (0.0-100.0)
    };

    // 系统监控类
    class Monitor
    {
    public:
        Monitor();
        ~Monitor();

        // 禁止拷贝
        Monitor(const Monitor&) = delete;
        Monitor& operator=(const Monitor&) = delete;

        // 获取CPU使用率 (百分比)
        // 需要调用两次来计算差值，第一次调用返回0
        [[nodiscard]] double GetCpuUsage() const;

        // 获取内存使用率 (百分比)
        [[nodiscard]] double GetMemoryUsage() const;

        // 获取已使用内存 (MB)
        [[nodiscard]] uint64_t GetMemoryUsedMB() const;

        // 获取总内存 (MB)
        [[nodiscard]] uint64_t GetMemoryTotalMB() const;

        // 获取完整的系统指标
        [[nodiscard]] SystemMetrics GetSystemMetrics() const;

        // 获取指定路径的磁盘信息
        [[nodiscard]] DiskInfo GetDiskInfo(const std::string& path = "/") const;

        // 获取所有磁盘分区信息
        [[nodiscard]] std::vector<DiskInfo> GetAllDiskInfo() const;

        // 获取系统负载平均值 (仅Linux)
        [[nodiscard]] std::vector<double> GetLoadAverage() const;

        // 获取进程数
        [[nodiscard]] int GetProcessCount() const;

        // 获取系统运行时间 (秒)
        [[nodiscard]] uint64_t GetSystemUptime() const;

    private:
        class Impl;
        Impl* impl_;
    };
}
