#include "cppkit/strings.hpp"
#include <sstream>

namespace cppkit
{

  std::string trim(const std::string& s)
  {
    const auto start = s.find_first_not_of(" \t\n\r");
    const auto end   = s.find_last_not_of(" \t\n\r");
    if (start == std::string::npos)
    {
      return "";
    }
    return s.substr(start, end - start + 1);
  }

  std::string join(const std::vector<std::string>& list, const std::string& sep)
  {
    std::ostringstream oss;
    for (size_t i = 0; i < list.size(); ++i)
    {
      if (i > 0) oss << sep;
      oss << list[i];
    }
    return oss.str();
  }

  bool startsWith(const std::string& s, const std::string& prefix)
  {
    return s.size() >= prefix.size() && s.compare(0, prefix.size(), prefix) == 0;
  }

  bool endsWith(const std::string& s, const std::string& suffix)
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

  std::vector<std::string> split(const std::string& s, const char delimiter)
  {
    std::vector<std::string> result;
    size_t start = 0;
    size_t end   = 0;

    while ((end = s.find(delimiter, start)) != std::string::npos)
    {
      result.emplace_back(s.substr(start, end - start));
      start = end + 1;
    }

    result.emplace_back(s.substr(start));
    return result;
  }

  std::string replaceAll(std::string s, const std::string& from, const std::string& to)
  {
    size_t pos = 0;
    while ((pos = s.find(from, pos)) != std::string::npos)
    {
      s.replace(pos, from.length(), to);
      pos += to.length();
    }
    return s;
  }

}  // namespace cppkit