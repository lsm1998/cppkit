#include "cppkit/monitor.hpp"
#include "cppkit/platform.hpp"
#include <sstream>
#include <fstream>
#include <filesystem>
#include <algorithm>

#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
    #include <psapi.h>
    #include <fileapi.h>
#elif defined(__APPLE__)
    #include <mach/mach.h>
    #include <mach/processor_info.h>
    #include <mach/mach_host.h>
    #include <mach/vm_statistics.h>
    #include <sys/types.h>
    #include <sys/sysctl.h>
    #include <sys/param.h>
    #include <sys/mount.h>
#else
    #include <sys/statvfs.h>
    #include <unistd.h>
#endif

namespace cppkit
{
    // 跨平台实现
    class Monitor::Impl
    {
    public:
        Impl() : last_cpu_total_(0), last_cpu_idle_(0)
        {
            // 初始化时读取一次CPU数据
            [[maybe_unused]] const bool success = ReadCpuStats(last_cpu_total_, last_cpu_idle_);
        }

        [[nodiscard]] double GetCpuUsage() const
        {
            uint64_t cpu_total = 0, cpu_idle = 0;

            if (!ReadCpuStats(cpu_total, cpu_idle))
            {
                return 0.0;
            }

            const uint64_t total_delta = cpu_total - last_cpu_total_;
            const uint64_t idle_delta = cpu_idle - last_cpu_idle_;

            // 更新上次的数据
            last_cpu_total_ = cpu_total;
            last_cpu_idle_ = cpu_idle;

            if (total_delta == 0)
            {
                return 0.0;
            }

            const double usage = 100.0 * (1.0 - static_cast<double>(idle_delta) / static_cast<double>(total_delta));
            return std::clamp(usage, 0.0, 100.0);
        }

        [[nodiscard]] double GetMemoryUsage() const
        {
            const auto [used, total] = GetMemoryInfo();
            if (total == 0)
            {
                return 0.0;
            }
            return 100.0 * static_cast<double>(used) / static_cast<double>(total);
        }

        [[nodiscard]] uint64_t GetMemoryUsedMB() const
        {
            const auto [used, total] = GetMemoryInfo();
            return used;
        }

        [[nodiscard]] uint64_t GetMemoryTotalMB() const
        {
            const auto [used, total] = GetMemoryInfo();
            return total;
        }

        [[nodiscard]] SystemMetrics GetSystemMetrics() const
        {
            SystemMetrics metrics;
            metrics.cpu_usage_percent = GetCpuUsage();
            metrics.memory_usage_percent = GetMemoryUsage();
            metrics.memory_used_mb = GetMemoryUsedMB();
            metrics.memory_total_mb = GetMemoryTotalMB();
            return metrics;
        }

        [[nodiscard]] DiskInfo GetDiskInfo(const std::string& path) const
        {
#ifdef _WIN32
            return GetDiskInfoWindows(path);
#else
            return GetDiskInfoUnix(path);
#endif
        }

        [[nodiscard]] std::vector<DiskInfo> GetAllDiskInfo() const
        {
            std::vector<DiskInfo> disks;
#ifdef _WIN32
            // Windows: 获取所有逻辑驱动器
            const DWORD drives = GetLogicalDrives();
            for (int i = 0; i < 26; ++i)
            {
                if (drives & (1 << i))
                {
                    char path[] = {static_cast<char>('A' + i), ':', '\\', '\0'};
                    const UINT driveType = GetDriveTypeA(path);
                    if (driveType == DRIVE_FIXED || driveType == DRIVE_REMOVABLE)
                    {
                        DiskInfo info = GetDiskInfoWindows(path);
                        if (info.total_mb > 0)
                        {
                            disks.push_back(info);
                        }
                    }
                }
            }
#elif defined(__APPLE__)
            // macOS: 使用getmntinfo获取挂载点
            struct statfs* mounts;
            const int count = getmntinfo(&mounts, MNT_WAIT);
            for (int i = 0; i < count; ++i)
            {
                DiskInfo info = GetDiskInfoUnix(mounts[i].f_mntonname);
                if (info.total_mb > 0)
                {
                    disks.push_back(info);
                }
            }
#else
            // Linux: 读取/proc/mounts
            std::ifstream mounts("/proc/mounts");
            std::string line;
            while (std::getline(mounts, line))
            {
                std::istringstream iss(line);
                std::string device, mount_point;
                iss >> device >> mount_point;

                // 跳过特殊文件系统，只关注真实设备
                if (device.find("/dev/") == 0 || device.find("/dev/sd") == 0 ||
                    device.find("/dev/nvme") == 0 || device.find("/dev/mapper") == 0)
                {
                    DiskInfo info = GetDiskInfoUnix(mount_point);
                    if (info.total_mb > 0)
                    {
                        disks.push_back(info);
                    }
                }
            }
#endif
            return disks;
        }

        [[nodiscard]] std::vector<double> GetLoadAverage() const
        {
#ifdef _WIN32
            return {0.0, 0.0, 0.0};
#else
            std::vector<double> loadavg(3);
            if (getloadavg(loadavg.data(), 3) != 3)
            {
                return {0.0, 0.0, 0.0};
            }
            return loadavg;
#endif
        }

        [[nodiscard]] int GetProcessCount() const
        {
#ifdef _WIN32
            DWORD pid_count = 0;
            if (EnumProcesses(nullptr, 0, &pid_count))
            {
                return static_cast<int>(pid_count / sizeof(DWORD));
            }
            return 0;
#elif defined(__APPLE__)
            // macOS: 使用sysctl获取进程数
            int mib[4] = {CTL_KERN, KERN_PROC, KERN_PROC_ALL, 0};
            size_t len;
            sysctl(mib, 4, nullptr, &len, nullptr, 0);
            return static_cast<int>(len / sizeof(struct kinfo_proc));
#else
            // Linux: 遍历/proc目录
            int count = 0;
            const std::string proc_path = "/proc";
            for (const auto& entry : std::filesystem::directory_iterator(proc_path))
            {
                if (entry.is_directory())
                {
                    const std::string name = entry.path().filename().string();
                    if (std::all_of(name.begin(), name.end(), ::isdigit))
                    {
                        ++count;
                    }
                }
            }
            return count;
#endif
        }

        [[nodiscard]] uint64_t GetSystemUptime() const
        {
#ifdef _WIN32
            return GetTickCount64() / 1000;
#elif defined(__APPLE__)
            // macOS: 使用sysctl获取系统运行时间
            struct timeval boottime;
            size_t len = sizeof(boottime);
            int mib[2] = {CTL_KERN, KERN_BOOTTIME};
            if (sysctl(mib, 2, &boottime, &len, nullptr, 0) < 0)
            {
                return 0;
            }
            time_t boot_sec = boottime.tv_sec;
            time_t now_sec = time(nullptr);
            return static_cast<uint64_t>(now_sec - boot_sec);
#else
            // Linux: 读取/proc/uptime
            std::ifstream uptime("/proc/uptime");
            double up = 0.0;
            uptime >> up;
            return static_cast<uint64_t>(up);
#endif
        }

    private:
        mutable uint64_t last_cpu_total_;
        mutable uint64_t last_cpu_idle_;

        // 读取CPU统计信息
        [[nodiscard]] bool ReadCpuStats(uint64_t& total, uint64_t& idle) const
        {
#ifdef _WIN32
            return ReadCpuStatsWindows(total, idle);
#elif defined(__APPLE__)
            return ReadCpuStatsMac(total, idle);
#else
            return ReadCpuStatsLinux(total, idle);
#endif
        }

#ifdef _WIN32
        [[nodiscard]] bool ReadCpuStatsWindows(uint64_t& total, uint64_t& idle) const
        {
            FILETIME idle_time, kernel_time, user_time;
            if (GetSystemTimes(&idle_time, &kernel_time, &user_time))
            {
                auto filetime_to_uint64 = [](const FILETIME& ft)
                {
                    return (static_cast<uint64_t>(ft.dwHighDateTime) << 32) | ft.dwLowDateTime;
                };

                const uint64_t idle_ticks = filetime_to_uint64(idle_time);
                const uint64_t kernel_ticks = filetime_to_uint64(kernel_time);
                const uint64_t user_ticks = filetime_to_uint64(user_time);

                total = kernel_ticks + user_ticks;
                idle = idle_ticks;
                return true;
            }
            return false;
        }
#endif

#ifdef __APPLE__
        [[nodiscard]] bool ReadCpuStatsMac(uint64_t& total, uint64_t& idle) const
        {
            host_cpu_load_info_data_t cpu_info;
            mach_msg_type_number_t count = HOST_CPU_LOAD_INFO_COUNT;
            if (host_statistics(mach_host_self(), HOST_CPU_LOAD_INFO,
                               reinterpret_cast<host_info_t>(&cpu_info), &count) == KERN_SUCCESS)
            {
                // user, system, idle, nice
                const uint64_t user = cpu_info.cpu_ticks[CPU_STATE_USER];
                const uint64_t system = cpu_info.cpu_ticks[CPU_STATE_SYSTEM];
                const uint64_t idle_ticks = cpu_info.cpu_ticks[CPU_STATE_IDLE];
                const uint64_t nice = cpu_info.cpu_ticks[CPU_STATE_NICE];

                total = user + system + idle_ticks + nice;
                idle = idle_ticks;
                return true;
            }
            return false;
        }
#endif

#ifndef _WIN32
#ifndef __APPLE__
        [[nodiscard]] bool ReadCpuStatsLinux(uint64_t& total, uint64_t& idle) const
        {
            std::ifstream stat("/proc/stat");
            std::string line;
            if (!std::getline(stat, line))
            {
                return false;
            }

            std::istringstream iss(line);
            std::string cpu_label;
            iss >> cpu_label;

            uint64_t user = 0, nice = 0, system_time = 0, idle_time = 0;
            uint64_t iowait = 0, irq = 0, softirq = 0, steal = 0;

            iss >> user >> nice >> system_time >> idle_time >> iowait >> irq >> softirq >> steal;

            total = user + nice + system_time + idle_time + iowait + irq + softirq + steal;
            idle = idle_time;
            return true;
        }
#endif
#endif

        // 获取内存信息
        [[nodiscard]] std::pair<uint64_t, uint64_t> GetMemoryInfo() const
        {
#ifdef _WIN32
            return GetMemoryInfoWindows();
#elif defined(__APPLE__)
            return GetMemoryInfoMac();
#else
            return GetMemoryInfoLinux();
#endif
        }

#ifdef _WIN32
        [[nodiscard]] std::pair<uint64_t, uint64_t> GetMemoryInfoWindows() const
        {
            MEMORYSTATUSEX status;
            status.dwLength = sizeof(status);
            if (GlobalMemoryStatusEx(&status))
            {
                const uint64_t total_mb = status.ullTotalPhys / (1024 * 1024);
                const uint64_t used_mb = (status.ullTotalPhys - status.ullAvailPhys) / (1024 * 1024);
                return {used_mb, total_mb};
            }
            return {0, 0};
        }
#endif

#ifdef __APPLE__
        [[nodiscard]] std::pair<uint64_t, uint64_t> GetMemoryInfoMac() const
        {
            // 使用sysctl获取内存信息
            int mib[2];
            size_t len;
            uint64_t total_mem = 0;
            uint64_t free_mem = 0;
            uint64_t inactive_mem = 0;
            uint64_t active_mem = 0;
            uint64_t wired_mem = 0;

            // 获取总内存
            mib[0] = CTL_HW;
            mib[1] = HW_MEMSIZE;
            len = sizeof(total_mem);
            sysctl(mib, 2, &total_mem, &len, nullptr, 0);

            // 获取VM统计信息
            vm_statistics64_data_t vm_stats;
            mach_msg_type_number_t count = HOST_VM_INFO64_COUNT;
            if (host_statistics64(mach_host_self(), HOST_VM_INFO64,
                                 reinterpret_cast<host_info64_t>(&vm_stats), &count) == KERN_SUCCESS)
            {
                // 在macOS上计算可用内存
                const int page_size = getpagesize();
                free_mem = vm_stats.free_count * page_size;
                inactive_mem = vm_stats.inactive_count * page_size;
                active_mem = vm_stats.active_count * page_size;
                wired_mem = vm_stats.wire_count * page_size;
            }

            // 计算已使用内存（总内存 - 空闲内存）
            // 注意：macOS的内存管理更复杂，这里使用简化计算
            const uint64_t available_mem = free_mem + inactive_mem;
            const uint64_t used_mem = total_mem - available_mem;

            const uint64_t total_mb = total_mem / (1024 * 1024);
            const uint64_t used_mb = used_mem / (1024 * 1024);

            return {used_mb, total_mb};
        }
#endif

#ifndef _WIN32
#ifndef __APPLE__
        [[nodiscard]] std::pair<uint64_t, uint64_t> GetMemoryInfoLinux() const
        {
            std::ifstream meminfo("/proc/meminfo");
            std::string line;
            uint64_t total = 0, available = 0;

            while (std::getline(meminfo, line))
            {
                std::istringstream iss(line);
                std::string key;
                uint64_t value;
                std::string unit;

                iss >> key >> value >> unit;

                if (key == "MemTotal:")
                {
                    total = value / 1024; // KB to MB
                }
                else if (key == "MemAvailable:")
                {
                    available = value / 1024; // KB to MB
                }
            }

            const uint64_t used = total - available;
            return {used, total};
        }
#endif
#endif

#ifdef _WIN32
        // Windows获取磁盘信息
        [[nodiscard]] DiskInfo GetDiskInfoWindows(const std::string& path) const
        {
            DiskInfo info;
            info.path = path;
            info.used_mb = 0;
            info.total_mb = 0;
            info.usage_percent = 0.0;

            ULARGE_INTEGER free_bytes, total_bytes, available_bytes;
            if (GetDiskFreeSpaceExA(path.c_str(), &free_bytes, &total_bytes, &available_bytes))
            {
                info.total_mb = static_cast<uint64_t>(total_bytes.QuadPart) / (1024 * 1024);
                info.used_mb = info.total_mb - static_cast<uint64_t>(free_bytes.QuadPart) / (1024 * 1024);
                info.usage_percent = info.total_mb > 0 ?
                    100.0 * static_cast<double>(info.used_mb) / static_cast<double>(info.total_mb) : 0.0;
            }
            return info;
        }
#endif

#ifndef _WIN32
        // Unix/Linux/macOS获取磁盘信息
        [[nodiscard]] DiskInfo GetDiskInfoUnix(const std::string& path) const
        {
            DiskInfo info;
            info.path = path;
            info.used_mb = 0;
            info.total_mb = 0;
            info.usage_percent = 0.0;

            struct statvfs stat;
            if (statvfs(path.c_str(), &stat) == 0)
            {
                const uint64_t total = stat.f_blocks * stat.f_frsize;
                const uint64_t available = stat.f_bavail * stat.f_frsize;
                const uint64_t used = total - available;

                info.total_mb = total / (1024 * 1024);
                info.used_mb = used / (1024 * 1024);
                info.usage_percent = total > 0 ? 100.0 * static_cast<double>(used) / static_cast<double>(total) : 0.0;
            }
            return info;
        }
#endif
    };

    // Monitor类实现
    Monitor::Monitor() : impl_(new Impl())
    {
    }

    Monitor::~Monitor()
    {
        delete impl_;
    }

    double Monitor::GetCpuUsage() const
    {
        return impl_->GetCpuUsage();
    }

    double Monitor::GetMemoryUsage() const
    {
        return impl_->GetMemoryUsage();
    }

    uint64_t Monitor::GetMemoryUsedMB() const
    {
        return impl_->GetMemoryUsedMB();
    }

    uint64_t Monitor::GetMemoryTotalMB() const
    {
        return impl_->GetMemoryTotalMB();
    }

    SystemMetrics Monitor::GetSystemMetrics() const
    {
        return impl_->GetSystemMetrics();
    }

    DiskInfo Monitor::GetDiskInfo(const std::string& path) const
    {
        return impl_->GetDiskInfo(path);
    }

    std::vector<DiskInfo> Monitor::GetAllDiskInfo() const
    {
        return impl_->GetAllDiskInfo();
    }

    std::vector<double> Monitor::GetLoadAverage() const
    {
        return impl_->GetLoadAverage();
    }

    int Monitor::GetProcessCount() const
    {
        return impl_->GetProcessCount();
    }

    uint64_t Monitor::GetSystemUptime() const
    {
        return impl_->GetSystemUptime();
    }
}
