#pragma once

#include <cctype>
#include <iomanip>
#include <map>
#include <sstream>
#include <stdexcept>
#include <vector>
#include <variant>
#include <functional>

namespace cppkit::json
{
  class Json
  {
  public:
    using array = std::vector<Json>;
    using object = std::map<std::string, Json>;
    using value_type = std::variant<std::nullptr_t, bool, double, std::string, array, object>;

    value_type v;

    Json() noexcept : v(nullptr)
    {
    }

    explicit Json(std::nullptr_t) noexcept : v(nullptr)
    {
    }

    explicit Json(bool b) noexcept : v(b)
    {
    }

    explicit Json(const int i) noexcept : v(static_cast<double>(i))
    {
    }

    explicit Json(const long l) noexcept : v(static_cast<double>(l))
    {
    }

    explicit Json(double d) noexcept : v(d)
    {
    }

    explicit Json(const char* s) : v(std::string(s))
    {
    }

    explicit Json(const std::string& s) : v(s)
    {
    }

    explicit Json(std::string&& s) : v(std::string(std::move(s)))
    {
    }

    explicit Json(const array& a) : v(a)
    {
    }

    explicit Json(array&& a) : v(std::move(a))
    {
    }

    explicit Json(const object& o) : v(o)
    {
    }

    explicit Json(object&& o) : v(std::move(o))
    {
    }

    static Json makeArray() { return Json(array{}); }

    static Json makeObject() { return Json(object{}); }

    [[nodiscard]]
    bool isNull() const { return std::holds_alternative<std::nullptr_t>(v); }

    [[nodiscard]]
    bool isBool() const { return std::holds_alternative<bool>(v); }

    [[nodiscard]]
    bool isNumber() const { return std::holds_alternative<double>(v); }

    [[nodiscard]]
    bool isString() const { return std::holds_alternative<std::string>(v); }

    [[nodiscard]]
    bool isArray() const { return std::holds_alternative<array>(v); }

    [[nodiscard]]
    bool isObject() const { return std::holds_alternative<object>(v); }

    bool& asBool() { return std::get<bool>(v); }

    double& asNumber() { return std::get<double>(v); }

    std::string& asString() { return std::get<std::string>(v); }

    array& asArray() { return std::get<array>(v); }

    object& asObject() { return std::get<object>(v); }

    [[nodiscard]]
    const bool& asBool() const { return std::get<bool>(v); }

    [[nodiscard]]
    const double& asNumber() const { return std::get<double>(v); }

    [[nodiscard]]
    const std::string& asString() const { return std::get<std::string>(v); }

    [[nodiscard]]
    const array& asArray() const { return std::get<array>(v); }

    [[nodiscard]]
    const object& asObject() const { return std::get<object>(v); }

    Json& operator[](const std::string& key)
    {
      if (!isObject())
        v = object{};
      return std::get<object>(v)[key];
    }

    const Json& operator[](const std::string& key) const { return std::get<object>(v).at(key); }

    Json& operator[](const size_t idx)
    {
      if (!isArray())
        throw std::runtime_error("not an array");
      return std::get<array>(v).at(idx);
    }

    // Assignment operators
    Json& operator=(std::nullptr_t) noexcept
    {
      v = nullptr;
      return *this;
    }

    Json& operator=(bool b) noexcept
    {
      v = b;
      return *this;
    }

    Json& operator=(const int i) noexcept
    {
      v = static_cast<double>(i);
      return *this;
    }

    Json& operator=(const long l) noexcept
    {
      v = static_cast<double>(l);
      return *this;
    }

    Json& operator=(double d) noexcept
    {
      v = d;
      return *this;
    }

    Json& operator=(const char* s)
    {
      v = std::string(s);
      return *this;
    }

    Json& operator=(const std::string& s)
    {
      v = s;
      return *this;
    }

    Json& operator=(std::string&& s)
    {
      v = std::move(s);
      return *this;
    }

    Json& operator=(const array& a)
    {
      v = a;
      return *this;
    }

    Json& operator=(array&& a)
    {
      v = std::move(a);
      return *this;
    }

    Json& operator=(const object& o)
    {
      v = o;
      return *this;
    }

    Json& operator=(object&& o)
    {
      v = std::move(o);
      return *this;
    }

    // Serialization
    [[nodiscard]]
    std::string dump(const bool pretty = false, const int indent_size = 2) const
    {
      std::ostringstream oss;
      if (pretty)
        dumpPretty(oss, 0, indent_size);
      else
        dumpCompact(oss);
      return oss.str();
    }

  private:
    static void escapeString(std::ostream& os, const std::string& s)
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
              os << "\\u" << std::hex << std::uppercase << std::setw(4) << std::setfill('0') << (int) c << std::dec
                  << std::nouppercase;
            }
            else
              os << c;
        }
      }
      os << '"';
    }

    void dumpCompact(std::ostream& os) const
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
        return;
      }
    }

    void dumpPretty(std::ostream& os, int depth, int indent_size) const
    {
      const std::string indent(depth * indent_size, ' ');
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
          os << std::string((depth + 1) * indent_size, ' ');
          a[i].dumpPretty(os, depth + 1, indent_size);
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
          os << std::string((depth + 1) * indent_size, ' ');
          escapeString(os, p.first);
          os << ": ";
          p.second.dumpPretty(os, depth + 1, indent_size);
        }
        os << '\n' << indent << '}';
        return;
      }
    }

  public:
    static Json parse(const std::string& s)
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
                // simple conversion: assume code < 0x80
                if (code < 0x80)
                  res.push_back(static_cast<char>(code));
                else
                {
                  // Note: proper UTF-8 encoding of >0x7F not fully implemented; we
                  // provide basic support
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
          if (idx >= s.size() || !std::isdigit((unsigned char) s[idx]))
            throw std::runtime_error("invalid number");
          while (idx < s.size() && std::isdigit((unsigned char) s[idx]))
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
  };
} // namespace cppkit::json