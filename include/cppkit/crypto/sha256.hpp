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

    void update(const uint8_t* data, size_t len)
    {
      while (len > 0)
      {
        const size_t toCopy = std::min(len, 64 - bufferLen_);
        std::memcpy(buffer_ + bufferLen_, data, toCopy);
        bufferLen_ += toCopy;
        data += toCopy;
        len -= toCopy;

        if (bufferLen_ == 64)
        {
          transform(buffer_);
          totalBits_ += 512;
          bufferLen_ = 0;
        }
      }
    }

    void update(const std::string& str) { update(reinterpret_cast<const uint8_t*>(str.data()), str.size()); }

    std::array<uint8_t, 32> digest()
    {
      finalize();
      std::array<uint8_t, 32> out{};
      for (int i = 0; i < 8; ++i)
      {
        out[i * 4] = (state_[i] >> 24) & 0xFF;
        out[i * 4 + 1] = (state_[i] >> 16) & 0xFF;
        out[i * 4 + 2] = (state_[i] >> 8) & 0xFF;
        out[i * 4 + 3] = state_[i] & 0xFF;
      }
      return out;
    }

    std::string hexDigest()
    {
      auto d = digest();
      std::ostringstream oss;
      for (auto b : d)
        oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(b);
      return oss.str();
    }

    void reset();

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