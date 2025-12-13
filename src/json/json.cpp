#include "cppkit/json/json.hpp"

namespace cppkit::json
{
    std::string Json::dump(const bool pretty, const int indent_size) const
    {
        std::ostringstream oss;
        if (pretty)
            dumpPretty(oss, 0, indent_size);
        else
            dumpCompact(oss);
        return oss.str();
    }

    void Json::escapeString(std::ostream& os, const std::string& s)
    {
        os << '"';
        for (unsigned char c : s)
        {
            switch (c)
            {
            case '"':
                os << "\\\"";
                break;
            case '\\':
                os << "\\\\";
                break;
            case '\b':
                os << "\\b";
                break;
            case '\f':
                os << "\\f";
                break;
            case '\n':
                os << "\\n";
                break;
            case '\r':
                os << "\\r";
                break;
            case '\t':
                os << "\\t";
                break;
            default:
                if (c < 0x20)
                {
                    os << "\\u" << std::hex << std::uppercase << std::setw(4) << std::setfill('0') << (int)c <<
                        std::dec
                        << std::nouppercase;
                }
                else
                    os << c;
            }
        }
        os << '"';
    }

    void Json::dumpCompact(std::ostream& os) const
    {
        if (isNull())
        {
            os << "null";
            return;
        }
        if (isBool())
        {
            os << (asBool() ? "true" : "false");
            return;
        }
        if (isNumber())
        {
            double d = asNumber();
            // write minimal representation
            std::ostringstream tmp;
            tmp << std::setprecision(15) << d;
            os << tmp.str();
            return;
        }
        if (isString())
        {
            escapeString(os, asString());
            return;
        }
        if (isArray())
        {
            os << '[';
            const auto& a = asArray();
            for (size_t i = 0; i < a.size(); ++i)
            {
                if (i)
                    os << ',';
                a[i].dumpCompact(os);
            }
            os << ']';
            return;
        }
        if (isObject())
        {
            os << '{';
            const auto& o = asObject();
            bool first = true;
            for (const auto& p : o)
            {
                if (!first)
                    os << ',';
                first = false;
                escapeString(os, p.first);
                os << ':';
                p.second.dumpCompact(os);
            }
            os << '}';
        }
    }

    void Json::dumpPretty(std::ostream& os, const int depth, const int indentSize) const
    {
        const std::string indent(depth * indentSize, ' ');
        if (isNull())
        {
            os << "null";
            return;
        }
        if (isBool())
        {
            os << (asBool() ? "true" : "false");
            return;
        }
        if (isNumber())
        {
            std::ostringstream tmp;
            tmp << std::setprecision(15) << asNumber();
            os << tmp.str();
            return;
        }
        if (isString())
        {
            escapeString(os, asString());
            return;
        }
        if (isArray())
        {
            os << "[\n";
            const auto& a = asArray();
            for (size_t i = 0; i < a.size(); ++i)
            {
                if (i)
                    os << ",\n";
                os << std::string((depth + 1) * indentSize, ' ');
                a[i].dumpPretty(os, depth + 1, indentSize);
            }
            os << '\n' << indent << ']';
            return;
        }
        if (isObject())
        {
            os << "{\n";
            const auto& o = asObject();
            bool first = true;
            for (const auto& p : o)
            {
                if (!first)
                    os << ",\n";
                first = false;
                os << std::string((depth + 1) * indentSize, ' ');
                escapeString(os, p.first);
                os << ": ";
                p.second.dumpPretty(os, depth + 1, indentSize);
            }
            os << '\n' << indent << '}';
        }
    }

    Json Json::parse(const std::string& s)
    {
        size_t idx = 0;
        auto skip = [&]()
        {
            while (idx < s.size() && std::isspace(static_cast<unsigned char>(s[idx])))
                ++idx;
        };

        std::function<Json()> parse_value;

        const std::function parseString = [&]() -> std::string
        {
            if (s[idx] != '"')
                throw std::runtime_error("expected string\n");
            ++idx; // skip '"'
            std::string res;
            while (idx < s.size())
            {
                const char c = s[idx++];
                if (c == '"')
                    break;
                if (c == '\\')
                {
                    if (idx >= s.size())
                        throw std::runtime_error("unterminated escape");
                    switch (const char e = s[idx++])
                    {
                    case '"':
                        res.push_back('"');
                        break;
                    case '\\':
                        res.push_back('\\');
                        break;
                    case '/':
                        res.push_back('/');
                        break;
                    case 'b':
                        res.push_back('\b');
                        break;
                    case 'f':
                        res.push_back('\f');
                        break;
                    case 'n':
                        res.push_back('\n');
                        break;
                    case 'r':
                        res.push_back('\r');
                        break;
                    case 't':
                        res.push_back('\t');
                        break;
                    case 'u':
                        {
                            if (idx + 4 > s.size())
                                throw std::runtime_error("invalid unicode escape");
                            int code = 0;
                            for (int i = 0; i < 4; ++i)
                            {
                                const char ch = s[idx++];
                                code <<= 4;
                                if (ch >= '0' && ch <= '9')
                                    code += ch - '0';
                                else if (ch >= 'a' && ch <= 'f')
                                    code += 10 + ch - 'a';
                                else if (ch >= 'A' && ch <= 'F')
                                    code += 10 + ch - 'A';
                                else
                                    throw std::runtime_error("invalid unicode hex");
                            }
                            if (code < 0x80)
                                res.push_back(static_cast<char>(code));
                            else
                            {
                                if (code <= 0x7FF)
                                {
                                    res.push_back(static_cast<char>(0xC0 | ((code >> 6) & 0x1F)));
                                    res.push_back(static_cast<char>(0x80 | (code & 0x3F)));
                                }
                                else
                                {
                                    res.push_back(static_cast<char>(0xE0 | ((code >> 12) & 0x0F)));
                                    res.push_back(static_cast<char>(0x80 | ((code >> 6) & 0x3F)));
                                    res.push_back(static_cast<char>(0x80 | (code & 0x3F)));
                                }
                            }
                        }
                        break;
                    default:
                        throw std::runtime_error(std::string("invalid escape: ") + e);
                    }
                }
                else
                {
                    res.push_back(c);
                }
            }
            return res;
        };

        const std::function parseNumber = [&]() -> double
        {
            const size_t start = idx;
            if (s[idx] == '-')
                ++idx;
            if (idx < s.size() && s[idx] == '0')
                ++idx;
            else
            {
                if (idx >= s.size() || !std::isdigit((unsigned char)s[idx]))
                    throw std::runtime_error("invalid number");
                while (idx < s.size() && std::isdigit((unsigned char)s[idx]))
                    ++idx;
            }
            if (idx < s.size() && s[idx] == '.')
            {
                ++idx;
                if (idx >= s.size() || !std::isdigit(static_cast<unsigned char>(s[idx])))
                    throw std::runtime_error("invalid number");
                while (idx < s.size() && std::isdigit(static_cast<unsigned char>(s[idx])))
                    ++idx;
            }
            if (idx < s.size() && (s[idx] == 'e' || s[idx] == 'E'))
            {
                ++idx;
                if (idx < s.size() && (s[idx] == '+' || s[idx] == '-'))
                    ++idx;
                if (idx >= s.size() || !std::isdigit(static_cast<unsigned char>(s[idx])))
                    throw std::runtime_error("invalid number");
                while (idx < s.size() && std::isdigit(static_cast<unsigned char>(s[idx])))
                    ++idx;
            }
            double val = 0.0;
            try
            {
                val = std::stod(s.substr(start, idx - start));
            }
            catch (...)
            {
                throw std::runtime_error("bad number conversion");
            }
            return val;
        };

        parse_value = [&]() -> Json
        {
            skip();
            if (idx >= s.size())
                throw std::runtime_error("unexpected end");
            char c = s[idx];
            if (c == 'n')
            {
                if (s.compare(idx, 4, "null") == 0)
                {
                    idx += 4;
                    return Json(nullptr);
                }
                throw std::runtime_error("invalid token");
            }
            if (c == 't')
            {
                if (s.compare(idx, 4, "true") == 0)
                {
                    idx += 4;
                    return Json(true);
                }
                throw std::runtime_error("invalid token");
            }
            if (c == 'f')
            {
                if (s.compare(idx, 5, "false") == 0)
                {
                    idx += 5;
                    return Json(false);
                }
                throw std::runtime_error("invalid token");
            }
            if (c == '"')
            {
                std::string str = parseString();
                return Json(str);
            }
            if (c == '[')
            {
                ++idx; // skip '['
                array arr;
                skip();
                if (idx < s.size() && s[idx] == ']')
                {
                    ++idx;
                    return Json(arr);
                }
                while (true)
                {
                    arr.emplace_back(parse_value());
                    skip();
                    if (idx >= s.size())
                        throw std::runtime_error("unexpected end in array");
                    if (s[idx] == ',')
                    {
                        ++idx;
                        continue;
                    }
                    if (s[idx] == ']')
                    {
                        ++idx;
                        break;
                    }
                    throw std::runtime_error("expected ',' or ']' in array");
                }
                return Json(std::move(arr));
            }
            if (c == '{')
            {
                ++idx; // skip '{'
                object obj;
                skip();
                if (idx < s.size() && s[idx] == '}')
                {
                    ++idx;
                    return Json(obj);
                }
                while (true)
                {
                    skip();
                    if (idx >= s.size() || s[idx] != '"')
                        throw std::runtime_error("expected string key in object");
                    std::string key = parseString();
                    skip();
                    if (idx >= s.size() || s[idx] != ':')
                        throw std::runtime_error("expected ':' after key");
                    ++idx;
                    Json val = parse_value();
                    obj.emplace(std::move(key), std::move(val));
                    skip();
                    if (idx >= s.size())
                        throw std::runtime_error("unexpected end in object");
                    if (s[idx] == ',')
                    {
                        ++idx;
                        continue;
                    }
                    if (s[idx] == '}')
                    {
                        ++idx;
                        break;
                    }
                    throw std::runtime_error("expected ',' or '}' in object");
                }
                return Json(std::move(obj));
            }
            // number
            if (c == '-' || std::isdigit(static_cast<unsigned char>(c)))
            {
                double d = parseNumber();
                return Json(d);
            }
            throw std::runtime_error(std::string("unexpected char: ") + c);
        };

        Json root = parse_value();
        while (idx < s.size() && std::isspace(static_cast<unsigned char>(s[idx])))
            ++idx;
        if (idx != s.size())
            throw std::runtime_error("extra characters after JSON value");
        return root;
    }
}
