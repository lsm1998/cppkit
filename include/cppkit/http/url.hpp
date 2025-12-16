#pragma once

#include <string>
#include <string_view>
#include <map>
#include <vector>
#include "cppkit/strings.hpp"

namespace cppkit::http
{
    class UrlCodec
    {
    public:
        // URL 编码
        static std::string encode(std::string_view str, bool spaceAsPlus = false);

        // URL 解码
        static std::string decode(std::string_view str, bool spaceAsPlus = true);
    };

    class UrlValue
    {
        using ValueMap = std::map<std::string, std::vector<std::string>, std::less<>>;

        ValueMap data;

    public:
        [[nodiscard]] std::string get(std::string_view key) const;

        void set(std::string_view key, std::string_view value);

        void add(std::string_view key, std::string_view value);

        void del(std::string_view key);

        [[nodiscard]] bool has(std::string_view key) const;

        [[nodiscard]] std::string encode() const;

        [[nodiscard]] const ValueMap& map();

        static UrlValue parseQuery(std::string_view query);
    };
} // namespace cppkit::http
