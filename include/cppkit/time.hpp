#pragma once

#include <chrono>
#include <sstream>
#include <iomanip>
#include <string>
#include <string_view>
#include <iostream>
#include <ctime>
#include <vector>
#include <cmath>

namespace cppkit
{
    using Duration = std::chrono::nanoseconds;

    using SysTimeNano = std::chrono::time_point<std::chrono::system_clock, std::chrono::nanoseconds>;

    constexpr auto Nanosecond = std::chrono::nanoseconds(1);
    constexpr auto Microsecond = std::chrono::microseconds(1);
    constexpr auto Millisecond = std::chrono::milliseconds(1);
    constexpr auto Second = std::chrono::seconds(1);
    constexpr auto Minute = std::chrono::minutes(1);
    constexpr auto Hour = std::chrono::hours(1);

    class Time
    {
    public:
        enum class TimeZoneType { Local, UTC };

        Time() : tp_(std::chrono::time_point_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now())),
                 tz_type_(TimeZoneType::Local)
        {
        }

        explicit Time(const SysTimeNano tp, const TimeZoneType type = TimeZoneType::Local)
            : tp_(tp), tz_type_(type)
        {
        }

        static Time Now()
        {
            const auto now = std::chrono::system_clock::now();
            return Time(std::chrono::time_point_cast<std::chrono::nanoseconds>(now), TimeZoneType::Local);
        }

        static Time Date(const int year, const int month, const int day, const int hour, const int min, const int sec,
                         const int nsec,
                         const std::string_view zoneName = "Local")
        {
            std::tm t = {};
            t.tm_year = year - 1900;
            t.tm_mon = month - 1;
            t.tm_mday = day;
            t.tm_hour = hour;
            t.tm_min = min;
            t.tm_sec = sec;
            t.tm_isdst = -1;

            const std::time_t tt = std::mktime(&t);

            const auto tp_sys = std::chrono::system_clock::from_time_t(tt);

            auto tp_nano = std::chrono::time_point_cast<std::chrono::nanoseconds>(tp_sys);

            tp_nano += std::chrono::nanoseconds(nsec);

            const TimeZoneType type = zoneName == "UTC" ? TimeZoneType::UTC : TimeZoneType::Local;
            return Time(tp_nano, type);
        }

        static Time Unix(const int64_t sec, const int64_t nsec)
        {
            const auto dur = std::chrono::seconds(sec) + std::chrono::nanoseconds(nsec);
            const SysTimeNano tp(dur);
            return Time(tp, TimeZoneType::Local);
        }

        [[nodiscard]] int Year() const { return GetTm().tm_year + 1900; }
        [[nodiscard]] int Month() const { return GetTm().tm_mon + 1; }
        [[nodiscard]] int Day() const { return GetTm().tm_mday; }
        [[nodiscard]] int HourPart() const { return GetTm().tm_hour; }
        [[nodiscard]] int MinutePart() const { return GetTm().tm_min; }
        [[nodiscard]] int SecondPart() const { return GetTm().tm_sec; }

        [[nodiscard]] int NanoPart() const
        {
            const auto secs = std::chrono::time_point_cast<std::chrono::seconds>(tp_);
            return static_cast<int>((tp_ - secs).count());
        }

        [[nodiscard]] int64_t Unix() const
        {
            return std::chrono::duration_cast<std::chrono::seconds>(tp_.time_since_epoch()).count();
        }

        [[nodiscard]] int64_t UnixNano() const
        {
            return tp_.time_since_epoch().count();
        }

        [[nodiscard]] std::string Location() const
        {
            return tz_type_ == TimeZoneType::UTC ? "UTC" : "Local";
        }

        [[nodiscard]] Time Add(const Duration d) const
        {
            return Time(tp_ + d, tz_type_);
        }

        [[nodiscard]] Time AddDate(const int years, const int months, const int days) const
        {
            std::tm t = GetTm();
            t.tm_year += years;
            t.tm_mon += months;
            t.tm_mday += days;
            const std::time_t tt = std::mktime(&t);

            const auto old_secs = std::chrono::time_point_cast<std::chrono::seconds>(tp_);
            const auto nanos = tp_ - old_secs;

            const auto new_tp_sys = std::chrono::system_clock::from_time_t(tt);
            const auto new_tp_nano = std::chrono::time_point_cast<std::chrono::nanoseconds>(new_tp_sys);

            return Time(new_tp_nano + nanos, tz_type_);
        }

        [[nodiscard]] Duration Sub(const Time& u) const
        {
            return tp_ - u.tp_;
        }

        [[nodiscard]] bool Equal(const Time& u) const
        {
            return tp_ == u.tp_;
        }

        [[nodiscard]] bool Before(const Time& u) const
        {
            return tp_ < u.tp_;
        }

        [[nodiscard]] bool After(const Time& u) const
        {
            return tp_ > u.tp_;
        }

        [[nodiscard]] Time In(const std::string_view zoneName) const
        {
            if (zoneName == "UTC") return UTC();
            return Local();
        }

        [[nodiscard]] Time UTC() const
        {
            return Time(tp_, TimeZoneType::UTC);
        }

        [[nodiscard]] Time Local() const
        {
            return Time(tp_, TimeZoneType::Local);
        }

        static Duration Since(const Time& t)
        {
            return Now().Sub(t);
        }

        static Duration Until(const Time& t)
        {
            return t.Sub(Now());
        }


        [[nodiscard]] std::string Format(std::string pattern = "%Y-%m-%d %H:%M:%S") const
        {
            std::tm t = GetTm();

            if (size_t f_pos = pattern.find("%f"); f_pos != std::string::npos)
            {
                // 计算当前存储的纳秒数
                auto secs = std::chrono::time_point_cast<std::chrono::seconds>(tp_);
                auto ns = (tp_ - secs).count();

                std::stringstream ss_nano;
                ss_nano << std::setw(9) << std::setfill('0') << ns;
                std::string ns_str = ss_nano.str();

                while ((f_pos = pattern.find("%f")) != std::string::npos)
                {
                    pattern.replace(f_pos, 2, ns_str);
                }
            }

            char buffer[128];
            if (std::strftime(buffer, sizeof(buffer), pattern.c_str(), &t))
            {
                return {buffer};
            }
            return "";
        }

        friend std::ostream& operator<<(std::ostream& os, const Time& t)
        {
            os << t.Format();
            return os;
        }

        static std::string ToString(Duration d)
        {
            if (d == Duration::zero()) return "0s";
            const bool negative = d < Duration::zero();
            if (negative) d = -d;
            const auto ns_count = d.count();
            std::stringstream ss;
            if (negative) ss << "-";

            if (ns_count < 1000)
            {
                ss << ns_count << "ns";
                return ss.str();
            }
            if (ns_count < 1000000)
            {
                FormatFloat(ss, static_cast<double>(ns_count) / 1000.0, "µs");
                return ss.str();
            }
            if (ns_count < 1000000000)
            {
                FormatFloat(ss, static_cast<double>(ns_count) / 1000000.0, "ms");
                return ss.str();
            }

            auto h = std::chrono::duration_cast<std::chrono::hours>(d);
            d -= h;
            auto m = std::chrono::duration_cast<std::chrono::minutes>(d);
            d -= m;
            double s = static_cast<double>(d.count()) / 1000000000.0;

            if (h.count() > 0) ss << h.count() << "h";
            if (m.count() > 0) ss << m.count() << "m";
            if (s > 0 || (h.count() == 0 && m.count() == 0)) FormatFloat(ss, s, "s");

            return ss.str();
        }

    private:
        [[nodiscard]] std::tm GetTm() const
        {
            std::time_t tt = std::chrono::system_clock::to_time_t(
                std::chrono::time_point_cast<std::chrono::system_clock::duration>(tp_)
            );
            std::tm res{};
            if (tz_type_ == TimeZoneType::UTC)
            {
#if defined(_WIN32)
                gmtime_s(&res, &tt);
#else
                gmtime_r(&tt, &res);
#endif
            }
            else
            {
#if defined(_WIN32)
                localtime_s(&res, &tt);
#else
                localtime_r(&tt, &res);
#endif
            }
            return res;
        }

        static void FormatFloat(std::stringstream& ss, const double val, const char* suffix)
        {
            std::stringstream tmp;
            tmp << std::fixed << std::setprecision(3) << val;
            std::string str = tmp.str();
            str.erase(str.find_last_not_of('0') + 1, std::string::npos);
            if (str.back() == '.') str.pop_back();
            ss << str << suffix;
        }

        SysTimeNano tp_;
        TimeZoneType tz_type_;
    };

    inline std::ostream& operator<<(std::ostream& os, const Duration& d)
    {
        os << Time::ToString(d);
        return os;
    }
}
