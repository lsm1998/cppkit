#pragma once

#include <string>
#include <vector>

namespace cppkit {

std::string trim(const std::string &s);

std::string join(const std::vector<std::string> &list, const std::string &sep);

bool startsWith(const std::string &s, const std::string &prefix);

bool endsWith(const std::string &s, const std::string &suffix);

std::string toLower(std::string s);

std::string toUpper(std::string s);

std::vector<std::string> split(const std::string &s, char delimiter);

std::string replaceAll(std::string s, const std::string &from,
                       const std::string &to);

} // namespace cppkit