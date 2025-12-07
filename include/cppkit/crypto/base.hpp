#pragma once

#include <string>
#include <vector>
#include <stdexcept>
#include <cstdint>

namespace cppkit::crypto
{
  class Base64 final
  {
  public:
    static std::string encode(const std::vector<uint8_t>& data);

    static std::string encode(const std::string& data);

    static std::vector<uint8_t> decode(const std::string& input);
  };
}