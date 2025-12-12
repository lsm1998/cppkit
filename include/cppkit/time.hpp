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

    struct Span
    {
        Duration d;

        explicit Span(const Duration dur) : d(dur)
        {
        }

        friend std::ostream& operator<<(std::ostream& os, const Span& s);

        explicit operator Duration() const { return d; }

        bool operator>(const Span& other) const { return d > other.d; }
        bool operator<(const Span& other) const { return d < other.d; }
        bool operator>=(const Span& other) const { return d >= other.d; }
        bool operator<=(const Span& other) const { return d <= other.d; }
        bool operator==(const Span& other) const { return d == other.d; }
    };

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

        static Time Now();

        static Time Date(int year, int month, int day, int hour, int min, int sec,
                         int nsec, std::string_view zoneName = "Local");

        static Time Unix(int64_t sec, int64_t nsec);

        [[nodiscard]] int Year() const { return GetTm().tm_year + 1900; }

        [[nodiscard]] int Month() const { return GetTm().tm_mon + 1; }

        [[nodiscard]] int Day() const { return GetTm().tm_mday; }

        [[nodiscard]] int HourPart() const { return GetTm().tm_hour; }

        [[nodiscard]] int MinutePart() const { return GetTm().tm_min; }

        [[nodiscard]] int SecondPart() const { return GetTm().tm_sec; }

        [[nodiscard]] int NanoPart() const;

        [[nodiscard]] int64_t Unix() const;

        [[nodiscard]] int64_t UnixNano() const;

        [[nodiscard]] std::string Location() const;

        [[nodiscard]] Time Add(Duration d) const;

        [[nodiscard]] Time AddDate(int years, int months, int days) const;

        [[nodiscard]] Duration Sub(const Time& u) const;

        [[nodiscard]] bool Equal(const Time& u) const;

        [[nodiscard]] bool Before(const Time& u) const;

        [[nodiscard]] bool After(const Time& u) const;

        [[nodiscard]] Time In(std::string_view zoneName) const;

        [[nodiscard]] Time UTC() const;

        [[nodiscard]] Time Local() const;

        static Span Since(const Time& t);

        static Span Until(const Time& t);


        [[nodiscard]] std::string Format(std::string pattern = "%Y-%m-%d %H:%M:%S") const;

        friend std::ostream& operator<<(std::ostream& os, const Time& t);

        static std::string ToString(Duration d);

    private:
        [[nodiscard]] std::tm GetTm() const;

        static void FormatFloat(std::stringstream& ss, double val, const char* suffix);

        SysTimeNano tp_;
        TimeZoneType tz_type_;
    };

    inline std::ostream& operator<<(std::ostream& os, const Span& s)
    {
        os << Time::ToString(s.d);
        return os;
    }
}
