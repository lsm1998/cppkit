#pragma once

#include <string>
#include <string_view>
#include <cctype>
#include <array>
#include <stdexcept>
#include <map>
#include <vector>

namespace cppkit::http
{
    class UrlCodec
    {
        static constexpr std::array<char, 16> hexLookup = {
            '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'
        };

        static constexpr int hexToInt(char c)
        {
            if (c >= '0' && c <= '9')
                return c - '0';
            if (c >= 'A' && c <= 'F')
                return c - 'A' + 10;
            if (c >= 'a' && c <= 'f')
                return c - 'a' + 10;
            return -1;
        }

    public:
        // URL 编码
        static std::string encode(std::string_view str, bool space_as_plus = false)
        {
            std::string result;

            result.reserve(str.size() * 1.5);

            for (unsigned char c : str)
            {
                if (std::isalnum(c) || c == '-' || c == '.' || c == '_' || c == '~')
                {
                    result += c;
                }
                else if (c == ' ' && space_as_plus)
                {
                    result += '+';
                }
                else
                {
                    result += '%';
                    result += hexLookup[c >> 4]; // 高4位
                    result += hexLookup[c & 0x0F]; // 低4位
                }
            }
            return result;
        }

        // URL 解码
        static std::string decode(std::string_view str, bool plus_as_space = true)
        {
            std::string result;
            result.reserve(str.size());

            for (size_t i = 0; i < str.length(); ++i)
            {
                if (str[i] == '%' && i + 2 < str.length())
                {
                    int high = hexToInt(str[i + 1]);
                    int low = hexToInt(str[i + 2]);

                    if (high != -1 && low != -1)
                    {
                        char decoded_char = static_cast<char>((high << 4) | low);
                        result += decoded_char;
                        i += 2;
                    }
                    else
                    {
                        result += str[i];
                    }
                }
                else if (str[i] == '+' && plus_as_space)
                {
                    result += ' ';
                }
                else
                {
                    result += str[i];
                }
            }
            return result;
        }
    };

    class UrlValue
    {
        using ValueMap = std::map<std::string, std::vector<std::string>, std::less<>>;

    private:
        ValueMap data;

    public:
        std::string get(std::string_view key) const
        {
            auto it = data.find(key);
            if (it == data.end() || it->second.empty())
            {
                return "";
            }
            return it->second.front();
        }

        void set(std::string_view key, std::string_view value) { data[std::string(key)] = {std::string(value)}; }

        void add(std::string_view key, std::string_view value) { data[std::string(key)].emplace_back(value); }

        void del(std::string_view key) { data.erase(std::string(key)); }

        bool has(std::string_view key) const
        {
            auto it = data.find(key);
            return it != data.end() && !it->second.empty();
        }

        std::string encode() const
        {
            if (data.empty())
            {
                return "";
            }

            std::string result;
            result.reserve(128);

            for (const auto& [key, vals] : data)
            {
                std::string encodedKey = UrlCodec::encode(key, true);

                for (const auto& val : vals)
                {
                    if (!result.empty())
                    {
                        result += '&';
                    }
                    result += encodedKey;
                    result += '=';
                    result += UrlCodec::encode(val, true);
                }
            }
            return result;
        }

        const ValueMap& map() const { return data; }

        static UrlValue parseQuery(std::string_view query)
        {
            UrlValue uv;
            size_t start = 0;
            while (start < query.length())
            {
                // 找到下一个 '&'
                size_t end = query.find('&', start);
                if (end == std::string_view::npos)
                {
                    end = query.length();
                }

                std::string_view pair = query.substr(start, end - start);
                if (!pair.empty())
                {
                    size_t eqPos = pair.find('=');
                    if (eqPos != std::string_view::npos)
                    {
                        std::string_view keyView = pair.substr(0, eqPos);
                        std::string_view valView = pair.substr(eqPos + 1);

                        std::string key = UrlCodec::decode(keyView, true);
                        std::string val = UrlCodec::decode(valView, true);

                        uv.add(key, val);
                    }
                    else
                    {
                        std::string key = UrlCodec::decode(pair, true);
                        uv.add(key, "");
                    }
                }

                start = end + 1;
            }
            return uv;
        }
    };
} // namespace cppkit::http
