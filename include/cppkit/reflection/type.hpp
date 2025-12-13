#pragma once

#include <string>

namespace cppkit::reflection
{
    template <typename T>
    constexpr std::string_view getTypeName()
    {
        std::string_view name;
#if defined(__clang__)
        name = __PRETTY_FUNCTION__;
        // Clang 格式: ... [T = int]
        constexpr std::string_view prefix = "T = ";
        constexpr std::string_view suffix = "]";
#elif defined(__GNUC__)
        name = __PRETTY_FUNCTION__;
        // GCC 格式: ... [with T = int]
        constexpr std::string_view prefix = "with T = ";
        constexpr std::string_view suffix = "]";
#elif defined(_MSC_VER)
        name = __FUNCSIG__;
        // MSVC 格式: ... getTypeName<int>(void)
        // MSVC 比较特殊，它的类型在尖括号里，且没有固定的 T= 前缀
        constexpr std::string_view prefix = "getTypeName<";
        constexpr std::string_view suffix = ">(void)";
#else
#error "Unsupported compiler"
#endif

        size_t start = name.find(prefix);
        if (start == std::string_view::npos) return "Unknown";
        start += prefix.size();

        const size_t end = name.rfind(suffix);
        if (end == std::string_view::npos) return "Unknown";

        if (start < end)
        {
            return name.substr(start, end - start);
        }
        return "Unknown";
    }
}
