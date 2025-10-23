#pragma once

#include <cctype>
#include <iomanip>
#include <map>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace cppkit::json {
class Json {
public:
  using array = std::vector<Json>;
  using object = std::map<std::string, Json>;
  using value_type =
      std::variant<std::nullptr_t, bool, double, std::string, array, object>;

  value_type v;

  Json() noexcept : v(nullptr) {}
  Json(std::nullptr_t) noexcept : v(nullptr) {}
  Json(bool b) noexcept : v(b) {}
  Json(int i) noexcept : v(static_cast<double>(i)) {}
  Json(long l) noexcept : v(static_cast<double>(l)) {}
  Json(double d) noexcept : v(d) {}
  Json(const char *s) : v(std::string(s)) {}
  Json(const std::string &s) : v(s) {}
  Json(std::string &&s) : v(std::string(std::move(s))) {}
  Json(const array &a) : v(a) {}
  Json(array &&a) : v(std::move(a)) {}
  Json(const object &o) : v(o) {}
  Json(object &&o) : v(std::move(o)) {}

  static Json make_array() { return Json(array{}); }
  static Json make_object() { return Json(object{}); }

  bool is_null() const { return std::holds_alternative<std::nullptr_t>(v); }
  bool is_bool() const { return std::holds_alternative<bool>(v); }
  bool is_number() const { return std::holds_alternative<double>(v); }
  bool is_string() const { return std::holds_alternative<std::string>(v); }
  bool is_array() const { return std::holds_alternative<array>(v); }
  bool is_object() const { return std::holds_alternative<object>(v); }

  bool &as_bool() { return std::get<bool>(v); }
  double &as_number() { return std::get<double>(v); }
  std::string &as_string() { return std::get<std::string>(v); }
  array &as_array() { return std::get<array>(v); }
  object &as_object() { return std::get<object>(v); }

  const bool &as_bool() const { return std::get<bool>(v); }
  const double &as_number() const { return std::get<double>(v); }
  const std::string &as_string() const { return std::get<std::string>(v); }
  const array &as_array() const { return std::get<array>(v); }
  const object &as_object() const { return std::get<object>(v); }

  Json &operator[](const std::string &key) {
    if (!is_object())
      v = object{};
    return std::get<object>(v)[key];
  }

  const Json &operator[](const std::string &key) const {
    return std::get<object>(v).at(key);
  }

  Json &operator[](size_t idx) {
    if (!is_array())
      throw std::runtime_error("not an array");
    return std::get<array>(v).at(idx);
  }

  // Serialization
  std::string dump(bool pretty = false, int indent_size = 2) const {
    std::ostringstream oss;
    if (pretty)
      dump_pretty(oss, 0, indent_size);
    else
      dump_compact(oss);
    return oss.str();
  }

private:
  static void escape_string(std::ostream &os, const std::string &s) {
    os << '"';
    for (unsigned char c : s) {
      switch (c) {
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
        if (c < 0x20) {
          os << "\\u" << std::hex << std::uppercase << std::setw(4)
             << std::setfill('0') << (int)c << std::dec << std::nouppercase;
        } else
          os << c;
      }
    }
    os << '"';
  }

  void dump_compact(std::ostream &os) const {
    if (is_null()) {
      os << "null";
      return;
    }
    if (is_bool()) {
      os << (as_bool() ? "true" : "false");
      return;
    }
    if (is_number()) {
      double d = as_number();
      // write minimal representation
      std::ostringstream tmp;
      tmp << std::setprecision(15) << d;
      os << tmp.str();
      return;
    }
    if (is_string()) {
      escape_string(os, as_string());
      return;
    }
    if (is_array()) {
      os << '[';
      const auto &a = as_array();
      for (size_t i = 0; i < a.size(); ++i) {
        if (i)
          os << ',';
        a[i].dump_compact(os);
      }
      os << ']';
      return;
    }
    if (is_object()) {
      os << '{';
      const auto &o = as_object();
      bool first = true;
      for (const auto &p : o) {
        if (!first)
          os << ',';
        first = false;
        escape_string(os, p.first);
        os << ':';
        p.second.dump_compact(os);
      }
      os << '}';
      return;
    }
  }

  void dump_pretty(std::ostream &os, int depth, int indent_size) const {
    std::string indent(depth * indent_size, ' ');
    if (is_null()) {
      os << "null";
      return;
    }
    if (is_bool()) {
      os << (as_bool() ? "true" : "false");
      return;
    }
    if (is_number()) {
      std::ostringstream tmp;
      tmp << std::setprecision(15) << as_number();
      os << tmp.str();
      return;
    }
    if (is_string()) {
      escape_string(os, as_string());
      return;
    }
    if (is_array()) {
      os << "[\n";
      const auto &a = as_array();
      for (size_t i = 0; i < a.size(); ++i) {
        if (i)
          os << ",\n";
        os << std::string((depth + 1) * indent_size, ' ');
        a[i].dump_pretty(os, depth + 1, indent_size);
      }
      os << '\n' << indent << ']';
      return;
    }
    if (is_object()) {
      os << "{\n";
      const auto &o = as_object();
      bool first = true;
      for (const auto &p : o) {
        if (!first)
          os << ",\n";
        first = false;
        os << std::string((depth + 1) * indent_size, ' ');
        escape_string(os, p.first);
        os << ": ";
        p.second.dump_pretty(os, depth + 1, indent_size);
      }
      os << '\n' << indent << '}';
      return;
    }
  }

public:
  static Json parse(const std::string &s) {
    size_t idx = 0;
    auto skip = [&](void) {
      while (idx < s.size() && std::isspace((unsigned char)s[idx]))
        ++idx;
    };

    std::function<Json()> parse_value;

    std::function<std::string()> parse_string = [&]() -> std::string {
      if (s[idx] != '"')
        throw std::runtime_error("expected string\n");
      ++idx; // skip '"'
      std::string res;
      while (idx < s.size()) {
        char c = s[idx++];
        if (c == '"')
          break;
        if (c == '\\') {
          if (idx >= s.size())
            throw std::runtime_error("unterminated escape");
          char e = s[idx++];
          switch (e) {
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
          case 'u': {
            if (idx + 4 > s.size())
              throw std::runtime_error("invalid unicode escape");
            int code = 0;
            for (int i = 0; i < 4; ++i) {
              char ch = s[idx++];
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
            else {
              // Note: proper UTF-8 encoding of >0x7F not fully implemented; we
              // provide basic support
              if (code <= 0x7FF) {
                res.push_back(static_cast<char>(0xC0 | ((code >> 6) & 0x1F)));
                res.push_back(static_cast<char>(0x80 | (code & 0x3F)));
              } else {
                res.push_back(static_cast<char>(0xE0 | ((code >> 12) & 0x0F)));
                res.push_back(static_cast<char>(0x80 | ((code >> 6) & 0x3F)));
                res.push_back(static_cast<char>(0x80 | (code & 0x3F)));
              }
            }
          } break;
          default:
            throw std::runtime_error(std::string("invalid escape: ") + e);
          }
        } else {
          res.push_back(c);
        }
      }
      return res;
    };

    std::function<double()> parse_number = [&]() -> double {
      size_t start = idx;
      if (s[idx] == '-')
        ++idx;
      if (idx < s.size() && s[idx] == '0')
        ++idx;
      else {
        if (idx >= s.size() || !std::isdigit((unsigned char)s[idx]))
          throw std::runtime_error("invalid number");
        while (idx < s.size() && std::isdigit((unsigned char)s[idx]))
          ++idx;
      }
      if (idx < s.size() && s[idx] == '.') {
        ++idx;
        if (idx >= s.size() || !std::isdigit((unsigned char)s[idx]))
          throw std::runtime_error("invalid number");
        while (idx < s.size() && std::isdigit((unsigned char)s[idx]))
          ++idx;
      }
      if (idx < s.size() && (s[idx] == 'e' || s[idx] == 'E')) {
        ++idx;
        if (idx < s.size() && (s[idx] == '+' || s[idx] == '-'))
          ++idx;
        if (idx >= s.size() || !std::isdigit((unsigned char)s[idx]))
          throw std::runtime_error("invalid number");
        while (idx < s.size() && std::isdigit((unsigned char)s[idx]))
          ++idx;
      }
      double val = 0.0;
      try {
        val = std::stod(s.substr(start, idx - start));
      } catch (...) {
        throw std::runtime_error("bad number conversion");
      }
      return val;
    };

    parse_value = [&]() -> Json {
      skip();
      if (idx >= s.size())
        throw std::runtime_error("unexpected end");
      char c = s[idx];
      if (c == 'n') {
        if (s.compare(idx, 4, "null") == 0) {
          idx += 4;
          return Json(nullptr);
        }
        throw std::runtime_error("invalid token");
      }
      if (c == 't') {
        if (s.compare(idx, 4, "true") == 0) {
          idx += 4;
          return Json(true);
        }
        throw std::runtime_error("invalid token");
      }
      if (c == 'f') {
        if (s.compare(idx, 5, "false") == 0) {
          idx += 5;
          return Json(false);
        }
        throw std::runtime_error("invalid token");
      }
      if (c == '"') {
        std::string str = parse_string();
        return Json(str);
      }
      if (c == '[') {
        ++idx; // skip '['
        Json::array arr;
        skip();
        if (idx < s.size() && s[idx] == ']') {
          ++idx;
          return Json(arr);
        }
        while (true) {
          arr.emplace_back(parse_value());
          skip();
          if (idx >= s.size())
            throw std::runtime_error("unexpected end in array");
          if (s[idx] == ',') {
            ++idx;
            continue;
          }
          if (s[idx] == ']') {
            ++idx;
            break;
          }
          throw std::runtime_error("expected ',' or ']' in array");
        }
        return Json(std::move(arr));
      }
      if (c == '{') {
        ++idx; // skip '{'
        Json::object obj;
        skip();
        if (idx < s.size() && s[idx] == '}') {
          ++idx;
          return Json(obj);
        }
        while (true) {
          skip();
          if (idx >= s.size() || s[idx] != '"')
            throw std::runtime_error("expected string key in object");
          std::string key = parse_string();
          skip();
          if (idx >= s.size() || s[idx] != ':')
            throw std::runtime_error("expected ':' after key");
          ++idx;
          Json val = parse_value();
          obj.emplace(std::move(key), std::move(val));
          skip();
          if (idx >= s.size())
            throw std::runtime_error("unexpected end in object");
          if (s[idx] == ',') {
            ++idx;
            continue;
          }
          if (s[idx] == '}') {
            ++idx;
            break;
          }
          throw std::runtime_error("expected ',' or '}' in object");
        }
        return Json(std::move(obj));
      }
      // number
      if (c == '-' || std::isdigit((unsigned char)c)) {
        double d = parse_number();
        return Json(d);
      }
      throw std::runtime_error(std::string("unexpected char: ") + c);
    };

    Json root = parse_value();
    while (idx < s.size() && std::isspace((unsigned char)s[idx]))
      ++idx;
    if (idx != s.size())
      throw std::runtime_error("extra characters after JSON value");
    return root;
  }
};
} // namespace cppkit::json