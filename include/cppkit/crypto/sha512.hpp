#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <sstream>
#include <iomanip>
#include <algorithm>

namespace cppkit::crypto
{
  class SHA512
  {
  public:
    SHA512() { reset(); }

    void update(const uint8_t* data, size_t len);

    void update(const std::string& str);

    std::array<uint8_t, 64> digest();

    std::string hexDigest();

    static std::string sha(const std::string& message);

    static std::string hmac(const std::string& key, const std::string& message);

  private:
    void reset();
    void finalize();
    void transform(const uint8_t block[128]);
    static uint64_t rotr(uint64_t x, uint64_t n);

    std::array<uint64_t, 8> state_{};
    std::array<uint8_t, 128> buffer_{};
    uint64_t bufferLen_{};
    uint64_t bitLen_{};
    bool finalized_{};
  };
} // namespace cppkit::crypto