#pragma once

#include <string>
#include <vector>
#include <algorithm>

namespace cppkit {

// Trims whitespace from both ends of the string
std::string trim(const std::string &s);

// Joins a vector of strings into a single string with the given separator
std::string join(const std::vector<std::string> &list, const std::string &sep);

// Checks if the string starts with the given prefix
bool startsWith(const std::string &s, const std::string &prefix);

// Checks if the string ends with the given suffix
bool endsWith(const std::string &s, const std::string &suffix);

// Converts the string to lowercase
std::string toLower(std::string s);

// Converts the string to uppercase
std::string toUpper(std::string s);

// Splits the string by the given delimiter into a vector of strings
std::vector<std::string> split(const std::string &s, char delimiter);

// Replaces all occurrences of 'from' with 'to' in the string
std::string replaceAll(std::string s, const std::string &from,
                       const std::string &to);

// Replaces up to maxReplaces occurrences of 'from' with 'to' in the string
std::string replace(std::string s, const std::string &from, const std::string &to, size_t maxReplaces);

// Escapes HTML special characters in the string
std::string escapeHtml(const std::string &s);

// Unescapes HTML special characters in the string
std::string unescapeHtml(const std::string &s);

// URL encodes the string
std::string urlEncode(const std::string &s);

// URL decodes the string
std::string urlDecode(const std::string &s);

} // namespace cppkit