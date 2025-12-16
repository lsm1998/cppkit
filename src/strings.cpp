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
    return replace(s, from, to, -1);
  }

  std::string replace(std::string s, const std::string& from, const std::string& to, size_t maxReplaces)
  {
    if(from.empty() || maxReplaces == 0)
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

  std::string escapeHtml(const std::string& s)
  {
    std::string result;
    result.reserve(s.size());

    for (char c : s)
    {
      switch (c)
      {
        case '&': result.append("&amp;"); break;
        case '<': result.append("&lt;"); break;
        case '>': result.append("&gt;"); break;
        case '"': result.append("&quot;"); break;
        case '\'': result.append("&#39;"); break;
        default: result.push_back(c); break;
      }
    }

    return result;
  }

  std::string unescapeHtml(const std::string& s)
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

}  // namespace cppkit