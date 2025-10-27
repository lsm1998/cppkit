#include "cppkit/arg_parser.hpp"
#include "cppkit/strings.hpp"
#include <sstream>

namespace cppkit {

void ArgParser::addOption(const std::string &name,
                          const std::string &description,
                          const std::string &defaultValue) {
  std::string key = normalizeKey(name);
  _options[key] = {key, description, defaultValue, false};
}

void ArgParser::addFlag(const std::string &name,
                        const std::string &description) {
  std::string key = normalizeKey(name);
  _options[key] = {key, description, "false", true};
}

void ArgParser::parse(int argc, char *argv[]) {
  _args.clear();
  for (int i = 1; i < argc; ++i) {
    _args.emplace_back(argv[i]);
  }

  for (size_t i = 0; i < _args.size(); ++i) {
    std::string arg = normalizeKey(_args[i]);
    auto it = _options.find(arg);
    if (it != _options.end()) {
      if (it->second.isFlag) {
        _values[arg] = "true";
      } else if (i + 1 < _args.size() && _args[i + 1][0] != '-') {
        _values[arg] = _args[i + 1];
        ++i; // 跳过值
      } else {
        _values[arg] = it->second.defaultValue; // 无值则用默认
      }
    }
  }
}

std::string ArgParser::getString(const std::string &name) const {
  auto it = _values.find(name);
  if (it != _values.end()) {
    return it->second;
  }

  auto def = _options.find(name);
  if (def != _options.end()) {
    return def->second.defaultValue;
  }

  return "";
}

bool ArgParser::has(const std::string &name) const {
  auto it = _values.find(normalizeKey(name));
  if (it != _values.end()) {
    return it->second == "true";
  }
  return false;
}

std::string ArgParser::help(const std::string &programName) const {
  std::ostringstream oss;
  if (!programName.empty()) {
    oss << "Usage: " << programName << " [options]\n\n";
  }
  oss << "Options:\n";
  for (const auto &[name, opt] : _options) {
    oss << "  " << name;
    if (!opt.isFlag) {
      oss << " <value>";
    }
    oss << "\n    " << opt.description;
    if (!opt.defaultValue.empty()) {
      oss << " (default: " << opt.defaultValue << ")";
    }
    oss << "\n";
  }
  return oss.str();
}

std::string ArgParser::normalizeKey(const std::string &key) const {
  if (startsWith(key, "--")) {
    return key.substr(2);
  } else if (startsWith(key, "-")) {
    return key.substr(1);
  }
  return key;
}

} // namespace cppkit