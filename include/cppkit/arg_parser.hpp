#pragma once

#include <string>
#include <unordered_map>
#include <vector>

namespace cppkit {

class ArgParser {
public:
  struct Option {
    std::string name;
    std::string description;
    std::string defaultValue;
    bool isFlag;
  };

  // 添加带值选项（如 --port 8080）
  void addOption(const std::string &name, const std::string &description = "",
                 const std::string &defaultValue = "");

  // 添加布尔开关（如 --verbose）
  void addFlag(const std::string &name, const std::string &description = "");

  // 解析命令行参数
  void parse(int argc, char *argv[]);

  // 获取参数值（若未提供，返回默认值）
  std::string get(const std::string &name) const;

  // 判断标志（布尔开关）是否存在
  bool has(const std::string &name) const;

  // 生成帮助信息
  std::string help(const std::string &programName = "") const;

private:
  std::unordered_map<std::string, Option> _options;
  std::unordered_map<std::string, std::string> _values;
  std::vector<std::string> _args;
};

} // namespace cppkit