#include "cppkit/time.hpp"

namespace cppkit
{
    Time Time::Now()
    {
        const auto now = std::chrono::system_clock::now();
        return Time(std::chrono::time_point_cast<std::chrono::nanoseconds>(now), TimeZoneType::Local);
    }

    Time Time::Date(const int year, const int month, const int day, const int hour, const int min, const int sec,
                    const int nsec, const std::string_view zoneName)
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

    Time Time::Unix(const int64_t sec, const int64_t nsec)
    {
        const auto dur = std::chrono::seconds(sec) + std::chrono::nanoseconds(nsec);
        const SysTimeNano tp(dur);
        return Time(tp, TimeZoneType::Local);
    }

    int Time::NanoPart() const
    {
        const auto secs = std::chrono::time_point_cast<std::chrono::seconds>(tp_);
        return static_cast<int>((tp_ - secs).count());
    }

    int64_t Time::Unix() const
    {
        return std::chrono::duration_cast<std::chrono::seconds>(tp_.time_since_epoch()).count();
    }

    int64_t Time::UnixNano() const
    {
        return tp_.time_since_epoch().count();
    }

    std::string Time::Location() const
    {
        return tz_type_ == TimeZoneType::UTC ? "UTC" : "Local";
    }

    Time Time::Add(const Duration d) const
    {
        return Time(tp_ + d, tz_type_);
    }

    Time Time::AddDate(const int years, const int months, const int days) const
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

    Duration Time::Sub(const Time& u) const
    {
        return tp_ - u.tp_;
    }

    bool Time::Equal(const Time& u) const
    {
        return tp_ == u.tp_;
    }

    bool Time::Before(const Time& u) const
    {
        return tp_ < u.tp_;
    }

    bool Time::After(const Time& u) const
    {
        return tp_ > u.tp_;
    }

    Time Time::In(const std::string_view zoneName) const
    {
        if (zoneName == "UTC") return UTC();
        return Local();
    }

    Time Time::UTC() const
    {
        return Time(tp_, TimeZoneType::UTC);
    }

    Time Time::Local() const
    {
        return Time(tp_, TimeZoneType::Local);
    }

    Span Time::Since(const Time& t)
    {
        return Span(Now().Sub(t));
    }

    Span Time::Until(const Time& t)
    {
        return Span(t.Sub(Now()));
    }

    std::string Time::Format(std::string pattern) const
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

    std::string Time::ToString(Duration d)
    {
        if (d == Duration::zero()) return "0s";
        const bool negative = d < Duration::zero();
        if (negative) d = -d;
        const auto nsCount = d.count();
        std::stringstream ss;
        if (negative) ss << "-";

        if (nsCount < 1000)
        {
            ss << nsCount << "ns";
            return ss.str();
        }
        if (nsCount < 1000000)
        {
            FormatFloat(ss, static_cast<double>(nsCount) / 1000.0, "µs");
            return ss.str();
        }
        if (nsCount < 1000000000)
        {
            FormatFloat(ss, static_cast<double>(nsCount) / 1000000.0, "ms");
            return ss.str();
        }

        const auto h = std::chrono::duration_cast<std::chrono::hours>(d);
        d -= h;
        const auto m = std::chrono::duration_cast<std::chrono::minutes>(d);
        d -= m;
        const double s = static_cast<double>(d.count()) / 1000000000.0;

        if (h.count() > 0) ss << h.count() << "h";
        if (m.count() > 0) ss << m.count() << "m";
        if (s > 0 || (h.count() == 0 && m.count() == 0)) FormatFloat(ss, s, "s");

        return ss.str();
    }

    std::tm Time::GetTm() const
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

    void Time::FormatFloat(std::stringstream& ss, const double val, const char* suffix)
    {
        std::stringstream tmp;
        tmp << std::fixed << std::setprecision(3) << val;
        std::string str = tmp.str();
        str.erase(str.find_last_not_of('0') + 1, std::string::npos);
        if (str.back() == '.') str.pop_back();
        ss << str << suffix;
    }

    std::ostream& operator<<(std::ostream& os, const Time& t)
    {
        os << t.Format();
        return os;
    }
}
