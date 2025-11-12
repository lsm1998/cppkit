#include "cppkit/crypto/md5.hpp"
#include <vector>
#include <iomanip>
#include <sstream>
#include <cstring>

namespace cppkit::crypto
{
  static uint32_t F(const uint32_t x, const uint32_t y, const uint32_t z) { return (x & y) | (~x & z); }
  static uint32_t G(const uint32_t x, const uint32_t y, const uint32_t z) { return (x & z) | (y & ~z); }
  static uint32_t H(const uint32_t x, const uint32_t y, const uint32_t z) { return x ^ y ^ z; }
  static uint32_t I(const uint32_t x, const uint32_t y, const uint32_t z) { return y ^ (x | ~z); }

  static uint32_t leftRotate(const uint32_t x, const uint32_t c) { return (x << c) | (x >> (32 - c)); }

  static constexpr uint32_t K[64] = {
      0xd76aa478, 0xe8c7b756, 0x242070db, 0xc1bdceee,
      0xf57c0faf, 0x4787c62a, 0xa8304613, 0xfd469501,
      0x698098d8, 0x8b44f7af, 0xffff5bb1, 0x895cd7be,
      0x6b901122, 0xfd987193, 0xa679438e, 0x49b40821,
      0xf61e2562, 0xc040b340, 0x265e5a51, 0xe9b6c7aa,
      0xd62f105d, 0x02441453, 0xd8a1e681, 0xe7d3fbc8,
      0x21e1cde6, 0xc33707d6, 0xf4d50d87, 0x455a14ed,
      0xa9e3e905, 0xfcefa3f8, 0x676f02d9, 0x8d2a4c8a,
      0xfffa3942, 0x8771f681, 0x6d9d6122, 0xfde5380c,
      0xa4beea44, 0x4bdecfa9, 0xf6bb4b60, 0xbebfbc70,
      0x289b7ec6, 0xeaa127fa, 0xd4ef3085, 0x04881d05,
      0xd9d4d039, 0xe6db99e5, 0x1fa27cf8, 0xc4ac5665,
      0xf4292244, 0x432aff97, 0xab9423a7, 0xfc93a039,
      0x655b59c3, 0x8f0ccc92, 0xffeff47d, 0x85845dd1,
      0x6fa87e4f, 0xfe2ce6e0, 0xa3014314, 0x4e0811a1,
      0xf7537e82, 0xbd3af235, 0x2ad7d2bb, 0xeb86d391
  };

  static const uint32_t S[64] = {
      7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22,
      5, 9, 14, 20, 5, 9, 14, 20, 5, 9, 14, 20, 5, 9, 14, 20,
      4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23,
      6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21
  };


  MD5::MD5() : bitLen_(0), finalized_(false)
  {
    state_[0] = 0x67452301;
    state_[1] = 0xefcdab89;
    state_[2] = 0x98badcfe;
    state_[3] = 0x10325476;
    std::memset(buffer_, 0, sizeof(buffer_));
  }

  void MD5::update(const uint8_t* data, const size_t len)
  {
    if (finalized_)
      return;
    size_t i = (bitLen_ / 8) % 64;
    bitLen_ += len * 8;

    const size_t partLen = 64 - i;
    size_t offset = 0;

    if (len >= partLen)
    {
      std::memcpy(&buffer_[i], data, partLen);
      transform(buffer_);
      offset = partLen;

      for (; offset + 63 < len; offset += 64)
        transform(&data[offset]);

      i = 0;
    }

    std::memcpy(&buffer_[i], &data[offset], len - offset);
  }

  void MD5::update(const std::string& data)
  {
    update(reinterpret_cast<const uint8_t*>(data.data()), data.size());
  }

  void MD5::finalize()
  {
    if (finalized_)
      return;

    constexpr uint8_t padding[64] = {0x80};
    uint8_t lengthBytes[8];

    for (int i = 0; i < 8; ++i)
      lengthBytes[i] = (bitLen_ >> (8 * i)) & 0xFF;

    const size_t index = (bitLen_ / 8) % 64;
    const size_t padLen = (index < 56) ? (56 - index) : (120 - index);

    update(padding, padLen);
    update(lengthBytes, 8);

    finalized_ = true;
  }

  void MD5::transform(const uint8_t block[64])
  {
    uint32_t a = state_[0], b = state_[1], c = state_[2], d = state_[3];
    uint32_t M[16];
    for (int i = 0; i < 16; ++i)
      M[i] = static_cast<uint32_t>(block[i * 4]) | (static_cast<uint32_t>(block[i * 4 + 1]) << 8) |
             (static_cast<uint32_t>(block[i * 4 + 2]) << 16) | (static_cast<uint32_t>(block[i * 4 + 3]) << 24);

    for (int i = 0; i < 64; ++i)
    {
      uint32_t Fv, g;
      if (i < 16)
      {
        Fv = F(b, c, d);
        g = i;
      }
      else if (i < 32)
      {
        Fv = G(b, c, d);
        g = (5 * i + 1) % 16;
      }
      else if (i < 48)
      {
        Fv = H(b, c, d);
        g = (3 * i + 5) % 16;
      }
      else
      {
        Fv = I(b, c, d);
        g = (7 * i) % 16;
      }

      const uint32_t tmp = d;
      d = c;
      c = b;
      b = b + leftRotate(a + Fv + K[i] + M[g], S[i]);
      a = tmp;
    }

    state_[0] += a;
    state_[1] += b;
    state_[2] += c;
    state_[3] += d;
  }

  std::array<uint8_t, 16> MD5::digest()
  {
    finalize();
    std::array<uint8_t, 16> out{};
    for (int i = 0; i < 4; ++i)
    {
      out[i * 4] = state_[i] & 0xFF;
      out[i * 4 + 1] = (state_[i] >> 8) & 0xFF;
      out[i * 4 + 2] = (state_[i] >> 16) & 0xFF;
      out[i * 4 + 3] = (state_[i] >> 24) & 0xFF;
    }
    return out;
  }

  std::string MD5::hexDigest()
  {
    const auto d = digest();
    std::ostringstream oss;
    for (const auto b : d)
      oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(b);
    return oss.str();
  }

  std::string MD5::base64Digest()
  {
    auto d = digest();
    return Base64::encode(std::vector(d.begin(), d.end()));
  }

  std::string MD5::hash(const std::string& data)
  {
    MD5 ctx;
    ctx.update(data);
    return ctx.hexDigest();
  }

  std::string MD5::hashBase64(const std::string& data)
  {
    MD5 ctx;
    ctx.update(data);
    return ctx.base64Digest();
  }
}