#pragma once

#include <chrono>
#include <sstream>
#include <iomanip>
#include <string>
#include <string_view>
#include <iostream>
#include <ctime>
#include <vector>

namespace cppkit
{
    using Duration = std::chrono::nanoseconds;
    
    constexpr Duration Nanosecond  = std::chrono::nanoseconds(1);
    constexpr Duration Microsecond = std::chrono::microseconds(1);
    constexpr Duration Millisecond = std::chrono::milliseconds(1);
    constexpr Duration Second      = std::chrono::seconds(1);
    constexpr Duration Minute      = std::chrono::minutes(1);
    constexpr Duration Hour        = std::chrono::hours(1);

    class Time
    {
    public:
        using ZonedTime = std::chrono::zoned_time<std::chrono::nanoseconds>;

        Time() : zt_(std::chrono::locate_zone("UTC"), std::chrono::sys_days{}) {}

        explicit Time(ZonedTime zt) : zt_(std::move(zt)) {}

        static Time Now()
        {
            return Time(ZonedTime(std::chrono::current_zone(), std::chrono::system_clock::now()));
        }

        static Time Date(int year, int month, int day, int hour, int min, int sec, int nsec, std::string_view zone_name = "UTC") 
        {
            auto zone = std::chrono::locate_zone(zone_name);
            
            std::chrono::year_month_day ymd(
                std::chrono::year(year), 
                std::chrono::month(month), 
                std::chrono::day(day)
            );
            
            if (!ymd.ok()) {
                throw std::runtime_error("Invalid date");
            }

            std::chrono::hh_mm_ss hms(
                std::chrono::hours(hour) + 
                std::chrono::minutes(min) + 
                std::chrono::seconds(sec) + 
                std::chrono::nanoseconds(nsec)
            );
            auto local = std::chrono::local_days(ymd) + hms.to_duration();
            
            try {
                return Time(ZonedTime(zone, local));
            } catch (const std::chrono::nonexistent_local_time&) {
                return Time(ZonedTime(zone, local + std::chrono::hours(1)));
            } catch (const std::chrono::ambiguous_local_time&) {
                return Time(ZonedTime(zone, local, std::chrono::choose::earliest));
            }
        }

        static Time Unix(int64_t sec, int64_t nsec) 
        {
            auto dur = std::chrono::seconds(sec) + std::chrono::nanoseconds(nsec);
            std::chrono::sys_time<std::chrono::nanoseconds> tp(dur);
            return Time(ZonedTime(std::chrono::current_zone(), tp));
        }

        [[nodiscard]] int Year() const 
        {
            auto ymd = std::chrono::year_month_day{std::chrono::floor<std::chrono::days>(zt_.get_local_time())};
            return static_cast<int>(ymd.year());
        }

        [[nodiscard]] int Month() const 
        {
            auto ymd = std::chrono::year_month_day{std::chrono::floor<std::chrono::days>(zt_.get_local_time())};
            return static_cast<unsigned>(ymd.month());
        }

        [[nodiscard]] int Day() const 
        {
            auto ymd = std::chrono::year_month_day{std::chrono::floor<std::chrono::days>(zt_.get_local_time())};
            return static_cast<unsigned>(ymd.day());
        }

        [[nodiscard]] int64_t Unix() const 
        {
            return std::chrono::duration_cast<std::chrono::seconds>(zt_.get_sys_time().time_since_epoch()).count();
        }

        [[nodiscard]] int64_t UnixNano() const 
        {
            return zt_.get_sys_time().time_since_epoch().count();
        }

        [[nodiscard]] std::string Location() const 
        {
            return std::string(zt_.get_time_zone()->name());
        }

        // 时间加法
        [[nodiscard]] Time Add(Duration d) const 
        {
            auto sys = zt_.get_sys_time() + d;
            return Time(ZonedTime(zt_.get_time_zone(), sys));
        }

        [[nodiscard]] Duration Sub(const Time& u) const 
        {
            return zt_.get_sys_time() - u.zt_.get_sys_time();
        }

        [[nodiscard]] Time AddDate(int years, int months, int days) const 
        {
            auto local = zt_.get_local_time();
            auto dp = std::chrono::floor<std::chrono::days>(local);
            auto time_part = local - dp;
            
            std::chrono::year_month_day ymd(dp);
            ymd += std::chrono::years(years);
            ymd += std::chrono::months(months);
            
            auto sys_d = std::chrono::sys_days(ymd) + std::chrono::days(days);
            auto new_local = sys_d + time_part;
            
            try {
                 return Time(ZonedTime(zt_.get_time_zone(), std::chrono::local_time<std::chrono::nanoseconds>(new_local)));
            } catch(const std::chrono::nonexistent_local_time&) {
                 return Time(ZonedTime(zt_.get_time_zone(), std::chrono::local_time<std::chrono::nanoseconds>(new_local) + std::chrono::hours(1)));
            } catch(...) {
                 return *this; 
            }
        }

        [[nodiscard]] bool Equal(const Time& u) const 
        {
            return zt_.get_sys_time() == u.zt_.get_sys_time();
        }

        [[nodiscard]] bool Before(const Time& u) const 
        {
            return zt_.get_sys_time() < u.zt_.get_sys_time();
        }

        [[nodiscard]] bool After(const Time& u) const 
        {
            return zt_.get_sys_time() > u.zt_.get_sys_time();
        }

        // 时区转换
        [[nodiscard]] Time In(std::string_view zone_name) const 
        {
            try {
                auto zone = std::chrono::locate_zone(zone_name);
                return Time(ZonedTime(zone, zt_.get_sys_time()));
            } catch (...) {
                return *this; 
            }
        }

        [[nodiscard]] Time UTC() const 
        {
            return In("UTC");
        }

        [[nodiscard]] Time Local() const 
        {
            return In(std::chrono::current_zone()->name());
        }

        inline Duration Since(const Time& t) 
        {
            return Time::Now().Sub(t);
        }

        inline Duration Until(const Time& t)
        {
            return t.Sub(Time::Now());
        }

        [[nodiscard]] std::string Format(std::string pattern = "%Y-%m-%d %H:%M:%S") const 
        {
            auto local_time = zt_.get_local_time();
            auto dp = std::chrono::floor<std::chrono::days>(local_time);
            std::chrono::year_month_day ymd{dp};
            std::chrono::hh_mm_ss hms{std::chrono::floor<std::chrono::nanoseconds>(local_time - dp)};

            std::tm t = {};
            t.tm_year = (int)ymd.year() - 1900;
            t.tm_mon  = (unsigned)ymd.month() - 1;
            t.tm_mday = (unsigned)ymd.day();
            t.tm_hour = hms.hours().count();
            t.tm_min  = hms.minutes().count();
            t.tm_sec  = hms.seconds().count();
            t.tm_isdst = -1; 

            size_t f_pos = pattern.find("%f");
            if (f_pos != std::string::npos) 
            {
                auto nanos = hms.subseconds().count(); 
                std::stringstream ss_nano;
                ss_nano << std::setw(9) << std::setfill('0') << nanos;
                std::string ns_str = ss_nano.str();
                
                while ((f_pos = pattern.find("%f")) != std::string::npos) 
                {
                    pattern.replace(f_pos, 2, ns_str);
                }
            }

            char buffer[256]; 
            if (std::strftime(buffer, sizeof(buffer), pattern.c_str(), &t)) 
            {
                return std::string(buffer);
            }
            return ""; 
        }

        friend std::ostream& operator<<(std::ostream& os, const Time& t) 
        {
            os << t.Format();
            return os;
        }

   
        inline std::string ToString(Duration d) {
            if (d == Duration::zero()) return "0s";

            bool negative = d < Duration::zero();
            if (negative) d = -d;

            std::stringstream ss;
            if (negative) ss << "-";

            auto ns = d.count();
        
            if (ns < 1000) {
                ss << ns << "ns";
                return ss.str();
            }
        
            if (ns < 1000000) {
                double us = static_cast<double>(ns) / 1000.0;
                ss << std::fixed << std::setprecision(3) << us;
                std::string s = ss.str();
                s.erase(s.find_last_not_of('0') + 1, std::string::npos);
                if (s.back() == '.') s.pop_back();
                return s + "µs";
            }

            if (ns < 1000000000) {
                double ms = static_cast<double>(ns) / 1000000.0;
                ss << std::fixed << std::setprecision(3) << ms;
                std::string s = ss.str();
                s.erase(s.find_last_not_of('0') + 1, std::string::npos);
                if (s.back() == '.') s.pop_back();
                return s + "ms";
            }

            auto h = std::chrono::duration_cast<std::chrono::hours>(d);
            if (h.count() > 0) {
                ss << h.count() << "h";
                d -= h;
            }

            auto m = std::chrono::duration_cast<std::chrono::minutes>(d);
            if (m.count() > 0 || h.count() > 0) {
                if (m.count() > 0) {
                    ss << m.count() << "m";
                    d -= m;
                }
            }

            double seconds = static_cast<double>(d.count()) / 1000000000.0;
            if (seconds > 0 || (h.count() == 0 && m.count() == 0)) {
                ss << std::fixed << std::setprecision(3) << seconds;
                std::string s = ss.str();
                std::string sec_str = s.substr(s.find_last_of("hm") == std::string::npos ? (negative ? 1 : 0) : s.find_last_of("hm") + 1);
                
                std::stringstream ss_sec;
                ss_sec << std::fixed << std::setprecision(3) << seconds;
                std::string pure_sec = ss_sec.str();
                pure_sec.erase(pure_sec.find_last_not_of('0') + 1, std::string::npos);
                if (pure_sec.back() == '.') pure_sec.pop_back();
                
                return ss.str().substr(0, ss.str().length() - (negative?0:0)) + pure_sec + "s";
            }
        
            std::string res = "";
            if (negative) res += "-";
            if (h.count() > 0) res += std::to_string(h.count()) + "h";
            if (m.count() > 0) res += std::to_string(m.count()) + "m";
        
            if (seconds > 0 || res.empty() || res == "-") {
                std::stringstream ss2;
                ss2 << std::fixed << std::setprecision(3) << seconds;
                std::string sec_str = ss2.str();
                sec_str.erase(sec_str.find_last_not_of('0') + 1, std::string::npos);
                if (sec_str.back() == '.') sec_str.pop_back();
                res += sec_str + "s";
            }
            return res;
        }

        inline std::ostream& operator<<(std::ostream& os, const Duration& d) {
            os << ToString(d);
            return os;
        }

    private:
        ZonedTime zt_;
    };
}