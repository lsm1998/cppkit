#pragma once

#include "strings.hpp"
#include <string>
#include <unordered_map>
#include <vector>

namespace cppkit
{
  class ArgParser
  {
  public:
    struct Option
    {
      std::string name;
      std::string description;
      std::string defaultValue;
      bool isFlag;
    };

    void addOption(const std::string& name,
        const std::string& description = "",
        const std::string& defaultValue = "");

    void addFlag(const std::string& name, const std::string& description = "");

    void parse(int argc, char* argv[]);

    [[nodiscard]] std::string getString(const std::string& name) const;

    template <typename T>
    T get(const std::string& name) const
    {
      std::string value = getString(name);
      if constexpr (std::is_same_v<T, std::string>)
      {
        return value;
      }
      else if constexpr (std::is_same_v<T, bool>)
      {
        const std::string lower = toLower(value);
        return (lower == "1" || lower == "true" || lower == "yes" ||
                lower == "on");
      }
      else if constexpr (std::is_integral_v<T>)
      {
        try
        {
          return static_cast<T>(std::stoll(value));
        }
        catch (...)
        {
          throw std::invalid_argument("Invalid integer value for option: " +
                                      name);
        }
      }
      else if constexpr (std::is_floating_point_v<T>)
      {
        try
        {
          return static_cast<T>(std::stod(value));
        }
        catch (...)
        {
          throw std::invalid_argument("Invalid floating value for option: " +
                                      name);
        }
      }
      else
      {
        static_assert(sizeof(T) == 0, "Unsupported type in ArgParser::get()");
      }
      return T{};
    }

    [[nodiscard]] bool has(const std::string& name) const;

    [[nodiscard]] std::string help(const std::string& programName = "") const;

  private:
    [[nodiscard]] std::string normalizeKey(const std::string& key) const;

    std::unordered_map<std::string, Option> _options;
    std::unordered_map<std::string, std::string> _values;
    std::vector<std::string> _args;
  };
} // namespace cppkit