#include "cppkit/http/url.hpp"

namespace cppkit::http
{
    // URL 编码
    std::string UrlCodec::encode(const std::string_view str, const bool spaceAsPlus)
    {
        return urlEncode(str, spaceAsPlus);
    }

    // URL 解码
    std::string UrlCodec::decode(const std::string_view str, const bool spaceAsPlus)
    {
        return urlDecode(str, spaceAsPlus);
    }

    std::string UrlValue::get(const std::string_view key) const
    {
        const auto it = data.find(key);
        if (it == data.end() || it->second.empty())
        {
            return "";
        }
        return it->second.front();
    }

    void UrlValue::set(const std::string_view key, const std::string_view value)
    {
        data[std::string(key)] = {std::string(value)};
    }

    void UrlValue::add(const std::string_view key, std::string_view value)
    {
        data[std::string(key)].emplace_back(value);
    }

    void UrlValue::del(const std::string_view key)
    {
        data.erase(std::string(key));
    }

    bool UrlValue::has(const std::string_view key) const
    {
        const auto it = data.find(key);
        return it != data.end() && !it->second.empty();
    }

    std::string UrlValue::encode() const
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

    const UrlValue::ValueMap& UrlValue::map() { return data; }

    UrlValue UrlValue::parseQuery(std::string_view query)
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

            if (std::string_view pair = query.substr(start, end - start); !pair.empty())
            {
                if (const size_t eqPos = pair.find('='); eqPos != std::string_view::npos)
                {
                    const std::string_view keyView = pair.substr(0, eqPos);
                    const std::string_view valView = pair.substr(eqPos + 1);

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
}
