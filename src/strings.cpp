#include "cppkit/strings.hpp"
#include <sstream>

namespace cppkit
{
    std::string trim(const std::string_view s)
    {
        const auto start = s.find_first_not_of(" \t\n\r");
        const auto end = s.find_last_not_of(" \t\n\r");
        if (start == std::string::npos)
        {
            return "";
        }
        return std::string(s.substr(start, end - start + 1));
    }

    std::string join(const std::vector<std::string>& list, const std::string_view sep)
    {
        std::ostringstream oss;
        for (size_t i = 0; i < list.size(); ++i)
        {
            if (i > 0) oss << sep;
            oss << list[i];
        }
        return oss.str();
    }

    bool startsWith(const std::string_view s, const std::string_view prefix)
    {
        return s.size() >= prefix.size() && s.compare(0, prefix.size(), prefix) == 0;
    }

    bool endsWith(const std::string_view s, const std::string_view suffix)
    {
        return s.size() >= suffix.size() && s.compare(s.size() - suffix.size(), suffix.size(), suffix) == 0;
    }

    std::string toLower(std::string s)
    {
        std::ranges::transform(s, s.begin(), [](const unsigned char c) { return std::tolower(c); });
        return s;
    }

    std::string toUpper(std::string s)
    {
        std::ranges::transform(s, s.begin(), [](const unsigned char c) { return std::toupper(c); });
        return s;
    }

    std::vector<std::string> split(std::string_view s, const char delimiter)
    {
        std::vector<std::string> result;
        size_t start = 0;
        size_t end = 0;

        while ((end = s.find(delimiter, start)) != std::string::npos)
        {
            result.emplace_back(s.substr(start, end - start));
            start = end + 1;
        }

        result.emplace_back(s.substr(start));
        return result;
    }

    std::string replaceAll(std::string s, std::string_view from, std::string_view to)
    {
        return replace(s, from, to, -1);
    }

    std::string replace(std::string s, const std::string_view from, const std::string_view to, const size_t maxReplaces)
    {
        if (from.empty() || maxReplaces == 0)
        {
            return s;
        }

        std::string result;
        result.reserve(s.size());

        size_t lastPos = 0;
        size_t findPos = 0;
        size_t count = 0;

        while ((findPos = s.find(from, lastPos)) != std::string::npos)
        {
            result.append(s, lastPos, findPos - lastPos);
            result.append(to);

            lastPos = findPos + from.length();
            if (++count == maxReplaces)
            {
                break;
            }
        }
        result.append(s, lastPos, std::string::npos);
        return result;
    }

    std::string escapeHtml(const std::string_view s)
    {
        std::string result;
        result.reserve(s.size());

        for (char c : s)
        {
            switch (c)
            {
            case '&': result.append("&amp;");
                break;
            case '<': result.append("&lt;");
                break;
            case '>': result.append("&gt;");
                break;
            case '"': result.append("&quot;");
                break;
            case '\'': result.append("&#39;");
                break;
            default: result.push_back(c);
                break;
            }
        }

        return result;
    }

    std::string unescapeHtml(const std::string_view s)
    {
        std::string res;
        res.reserve(s.size());

        for (size_t i = 0; i < s.size(); ++i)
        {
            if (s[i] == '&')
            {
                if (s.compare(i, 5, "&amp;") == 0)
                {
                    res.push_back('&');
                }
                else if (s.compare(i, 4, "&lt;") == 0)
                {
                    res.push_back('<');
                    i += 3;
                }
                else if (s.compare(i, 4, "&gt;") == 0)
                {
                    res.push_back('>');
                    i += 3;
                }
                else if (s.compare(i, 6, "&quot;") == 0)
                {
                    res.push_back('"');
                    i += 5;
                }
                else if (s.compare(i, 5, "&#39;") == 0)
                {
                    res.push_back('\'');
                    i += 4;
                }
                else
                {
                    res.push_back(s[i]);
                }
            }
            else
            {
                res.push_back(s[i]);
            }
        }

        return res;
    }

    inline int hexToInt(const char c)
    {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'a' && c <= 'f') return c - 'a' + 10;
        if (c >= 'A' && c <= 'F') return c - 'A' + 10;
        return -1;
    }

    std::string urlDecode(const std::string_view s, const bool spaceAsPlus)
    {
        std::string result;
        result.reserve(s.size());

        for (size_t i = 0; i < s.size(); ++i)
        {
            if (spaceAsPlus && s[i] == '+')
            {
                result += ' ';
            }
            else if (s[i] == '%')
            {
                if (i + 2 < s.size())
                {
                    const int high = hexToInt(s[i + 1]);
                    if (const int low = hexToInt(s[i + 2]); high != -1 && low != -1)
                    {
                        const unsigned char decodedChar = static_cast<unsigned char>(high) << 4
                            | static_cast<unsigned char>(low);
                        result += static_cast<char>(decodedChar);
                        i += 2; // 跳过后面两个字符
                    }
                    else
                    {
                        result += '%';
                    }
                }
                else
                {
                    result += '%';
                }
            }
            else
            {
                result += s[i];
            }
        }
        return result;
    }

    std::string urlEncode(const std::string_view s, const bool spaceAsPlus)
    {
        std::string result;
        result.reserve(s.size());

        for (size_t i = 0; i < s.size(); ++i)
        {
            if (spaceAsPlus && s[i] == '+')
            {
                result += ' ';
            }
            else if (s[i] == '%')
            {
                // 健壮性检查：确保后面还有两个字符
                if (i + 2 < s.size())
                {
                    int high = hexToInt(s[i + 1]);
                    int low = hexToInt(s[i + 2]);

                    if (high != -1 && low != -1)
                    {
                        const unsigned char decodedChar = static_cast<unsigned char>(high) << 4 |
                            static_cast<unsigned char>(low);
                        result += static_cast<char>(decodedChar);
                        i += 2;
                    }
                    else
                    {
                        result += '%';
                    }
                }
                else
                {
                    result += '%';
                }
            }
            else
            {
                result += s[i];
            }
        }
        return result;
    }
} // namespace cppkit
