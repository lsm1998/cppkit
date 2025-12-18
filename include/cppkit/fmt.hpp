#pragma once

#include <iostream>
#include <sstream>

namespace cppkit::inner
{
    constexpr size_t count_placeholders(const char* fmt, const size_t len)
    {
        size_t count = 0;
        for (size_t i = 0; i + 1 < len; ++i)
        {
            if (fmt[i] == '{' && fmt[i + 1] == '{')
            {
                // 转义 "{{" -> 输出 '{'，跳过
                ++i;
                continue;
            }
            if (fmt[i] == '}' && fmt[i + 1] == '}')
            {
                // 转义 "}}" -> 输出 '}'，跳过
                ++i;
                continue;
            }
            if (fmt[i] == '{' && fmt[i + 1] == '}')
            {
                ++count;
                ++i;
            }
        }
        return count;
    }

    template <size_t N>
    struct FormatString
    {
        char str[N]{};
        static constexpr size_t length = N - 1;

        constexpr FormatString(const char (&s)[N])
        {
            for (size_t i = 0; i < N; ++i)
                str[i] = s[i];
        }

        [[nodiscard]] constexpr size_t get_placeholder_count() const
        {
            return count_placeholders(str, N);
        }
    };

    inline void formatImpl(std::ostringstream& oss, const char* fmt)
    {
        while (*fmt)
        {
            if (fmt[0] == '{' && fmt[1] == '{')
            {
                oss << '{';
                fmt += 2;
                continue;
            }
            if (fmt[0] == '}' && fmt[1] == '}')
            {
                oss << '}';
                fmt += 2;
                continue;
            }
            oss << *fmt++;
        }
    }

    template <typename T, typename... Args>
    void formatImpl(std::ostringstream& oss, const char* fmt, const T& arg,
                    const Args&... args)
    {
        while (*fmt)
        {
            if (fmt[0] == '{' && fmt[1] == '{')
            {
                oss << '{';
                fmt += 2;
                continue;
            }
            if (fmt[0] == '}' && fmt[1] == '}')
            {
                oss << '}';
                fmt += 2;
                continue;
            }
            if (fmt[0] == '{' && fmt[1] == '}')
            {
                oss << arg;
                formatImpl(oss, fmt + 2, args...);
                return;
            }
            oss << *fmt++;
        }
    }

    template <FormatString Fmt, typename... Args>
    std::string safeFmtSprintfImpl(const Args&... args)
    {
        constexpr size_t expected = Fmt.get_placeholder_count();
        constexpr size_t actual = sizeof...(Args);

        static_assert(expected == actual,
                      "Number of placeholders does not match number of arguments");

        std::ostringstream oss;
        formatImpl(oss, Fmt.str, args...);
        return oss.str();
    }

    template <FormatString Fmt, typename... Args>
    void safeFmtPrintImpl(const Args&... args)
    {
        std::cout << safeFmtSprintfImpl<Fmt>(args...) << std::endl;
    }

    template <typename... Args>
    std::string runtimeFmtSprintfImpl(const char* fmt, const Args&... args)
    {
        std::ostringstream oss;
        formatImpl(oss, fmt, args...);
        return oss.str();
    }
} // namespace cppkit::inner


namespace cppkit
{
    template <typename... Args>
    std::string format(const char* fmt, Args&&... args)
    {
        return inner::runtimeFmtSprintfImpl(fmt, std::forward<Args>(args)...);
    }
}


#define print(fmt_str, ...) inner::safeFmtPrintImpl<fmt_str>(__VA_ARGS__)

#define sprintf(fmt_str, ...) inner::safeFmtSprintfImpl<fmt_str>(__VA_ARGS__)
