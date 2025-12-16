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
    // Type aliases for convenience
    using Duration = std::chrono::nanoseconds;

    // System time point with nanosecond precision
    using SysTimeNano = std::chrono::time_point<std::chrono::system_clock, std::chrono::nanoseconds>;

    // Common duration constants
    constexpr auto Nanosecond = std::chrono::nanoseconds(1);
    constexpr auto Microsecond = std::chrono::microseconds(1);
    constexpr auto Millisecond = std::chrono::milliseconds(1);
    constexpr auto Second = std::chrono::seconds(1);
    constexpr auto Minute = std::chrono::minutes(1);
    constexpr auto Hour = std::chrono::hours(1);

    // Span struct to represent time intervals
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

    // Time class representing a point in time
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

        // Returns the current local time
        static Time Now();

        // Creates a Time object from date and time components
        static Time Date(int year, int month, int day, int hour, int min, int sec,
                         int nsec, std::string_view zoneName = "Local");

        // Creates a Time object from Unix time in seconds and nanoseconds
        static Time Unix(int64_t sec, int64_t nsec);

        // Accessor methods for time components
        [[nodiscard]] int Year() const { return GetTm().tm_year + 1900; }

        // Month is 1-12
        [[nodiscard]] int Month() const { return GetTm().tm_mon + 1; }

        // Day of the month is 1-31
        [[nodiscard]] int Day() const { return GetTm().tm_mday; }

        // Weekday is 0-6 (Sunday = 0)
        [[nodiscard]] int Weekday() const { return GetTm().tm_wday; }

        // Day of the year is 1-366
        [[nodiscard]] int YearDay() const { return GetTm().tm_yday + 1; }

        // Hour part is 0-23
        [[nodiscard]] int HourPart() const { return GetTm().tm_hour; }

        // Minute part is 0-59
        [[nodiscard]] int MinutePart() const { return GetTm().tm_min; }

        // Second part is 0-60 (including leap second)
        [[nodiscard]] int SecondPart() const { return GetTm().tm_sec; }

        // Nanosecond part is 0-999,999,999
        [[nodiscard]] int NanoPart() const;

        // Returns Unix time in seconds
        [[nodiscard]] int64_t Unix() const;

        // Returns Unix time in nanoseconds
        [[nodiscard]] int64_t UnixNano() const;

        // Returns the time zone location name
        [[nodiscard]] std::string Location() const;

        // Adds a duration to the current time and returns a new Time object
        [[nodiscard]] Time Add(Duration d) const;

        // Adds years, months, and days to the current time and returns a new Time object
        [[nodiscard]] Time AddDate(int years, int months, int days) const;

        // Subtracts another Time from the current time and returns the duration between them
        [[nodiscard]] Duration Sub(const Time& u) const;

        // Comparison methods
        [[nodiscard]] bool Equal(const Time& u) const;

        // Returns true if the current time is before the given time
        [[nodiscard]] bool Before(const Time& u) const;

        // Returns true if the current time is after the given time
        [[nodiscard]] bool After(const Time& u) const;

        // Converts the time to the specified time zone
        [[nodiscard]] Time In(std::string_view zoneName) const;

        // Converts the time to UTC
        [[nodiscard]] Time UTC() const;

        // Converts the time to local time
        [[nodiscard]] Time Local() const;

        // Returns the duration since the given time
        [[nodiscard]]
        static Span Since(const Time& t);

        // Returns the duration until the given time
        [[nodiscard]]
        static Span Until(const Time& t);

        // Formats the time according to the given pattern
        [[nodiscard]] std::string Format(std::string pattern = "%Y-%m-%d %H:%M:%S") const;

        // Outputs the time to a stream
        friend std::ostream& operator<<(std::ostream& os, const Time& t);

        // Converts a duration to a human-readable string
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
