#pragma once

#include <array>
#include <cstdint>
#include <cstring>
#include <string>
#include <sstream>
#include <iomanip>
#include <vector>
#include <algorithm>

namespace cppkit::crypto
{
  class SHA256
  {
  public:
    SHA256() { reset(); }

    void update(const uint8_t* data, size_t len);

    void update(const std::string& str);

    std::array<uint8_t, 32> digest();

    std::string hexDigest();

    void reset();

    static std::string sha(const std::string& message);

    static std::string hmac(const std::string& key, const std::string& message);

  private:
    void finalize();

    void transform(const uint8_t block[64]);

    static uint32_t rotr(uint32_t x, uint32_t n);

    uint32_t state_[8]{};
    uint8_t buffer_[64]{};
    size_t bufferLen_ = 0;
    uint64_t totalBits_ = 0;
    bool finalized_ = false;
  };
} // namespace cppkit::crypto