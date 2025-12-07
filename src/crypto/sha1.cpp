#include "cppkit/crypto/sha1.hpp"
#include <vector>

namespace cppkit::crypto
{
  void SHA1::update(const uint8_t* data, size_t len)
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

  void SHA1::update(const std::string& str)
  {
    update(reinterpret_cast<const uint8_t*>(str.data()), str.size());
  }

  std::array<uint8_t, 20> SHA1::digest()
  {
    finalize();
    std::array<uint8_t, 20> out{};
    for (int i = 0; i < 5; ++i)
    {
      out[i * 4] = (state_[i] >> 24) & 0xFF;
      out[i * 4 + 1] = (state_[i] >> 16) & 0xFF;
      out[i * 4 + 2] = (state_[i] >> 8) & 0xFF;
      out[i * 4 + 3] = state_[i] & 0xFF;
    }
    return out;
  }

  std::string SHA1::hexDigest()
  {
    const auto d = digest();
    std::ostringstream oss;
    for (const auto b : d)
      oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(b);
    return oss.str();
  }

  void SHA1::reset()
  {
    state_[0] = 0x67452301;
    state_[1] = 0xEFCDAB89;
    state_[2] = 0x98BADCFE;
    state_[3] = 0x10325476;
    state_[4] = 0xC3D2E1F0;

    bufferLen_ = 0;
    totalBits_ = 0;
    finalized_ = false;
  }

  std::string SHA1::sha(const std::string& message)
  {
    SHA1 sha1;
    sha1.update(message);
    return sha1.hexDigest();
  }

  std::array<uint8_t, 20> SHA1::shaBinary(const std::string& message)
  {
    SHA1 sha1;
    sha1.update(message);
    return sha1.digest();
  }

  std::string SHA1::hmac(const std::string& key, const std::string& message)
  {
    std::vector<uint8_t> keyBytes(key.begin(), key.end());
    if (constexpr size_t blockSize = 64; keyBytes.size() > blockSize)
    {
      SHA1 sha1;
      sha1.update(key);
      auto hashedKey = sha1.digest();
      keyBytes = std::vector<uint8_t>(hashedKey.begin(), hashedKey.end());
    }
    constexpr size_t blockSize = 64;
    keyBytes.resize(blockSize, 0x00);
    std::vector<uint8_t> oKeyPad(blockSize);
    std::vector<uint8_t> iKeyPad(blockSize);
    for (size_t i = 0; i < blockSize; ++i)
    {
      oKeyPad[i] = keyBytes[i] ^ 0x5c;
      iKeyPad[i] = keyBytes[i] ^ 0x36;
    }

    SHA1 innerSha1;
    innerSha1.update(iKeyPad.data(), iKeyPad.size());
    innerSha1.update(reinterpret_cast<const uint8_t*>(message.data()), message.size());
    auto innerHash = innerSha1.digest();
    SHA1 outerSha1;
    outerSha1.update(oKeyPad.data(), oKeyPad.size());
    outerSha1.update(innerHash.data(), innerHash.size());
    auto hmacHash = outerSha1.digest();
    std::ostringstream oss;
    for (const auto b : hmacHash)
      oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(b);
    return oss.str();
  }

  void SHA1::finalize()
  {
    if (finalized_)
        return;

    // 处理剩余的buffer
    const uint64_t bitLength = totalBits_ + (bufferLen_ * 8);

    // 添加10000000的padding
    buffer_[bufferLen_++] = 0x80;

    // 如果剩余空间不足8字节存放长度，则处理当前块并创建一个新的全零块
    if (bufferLen_ > 56) {
        std::memset(buffer_ + bufferLen_, 0, 64 - bufferLen_);
        transform(buffer_);
        std::memset(buffer_, 0, sizeof(buffer_));
        bufferLen_ = 0;
    }

    // 填充剩余空间为0
    std::memset(buffer_ + bufferLen_, 0, 56 - bufferLen_);

    // 在块的最后8字节存储总长度（大端）
    buffer_[56] = (bitLength >> 56) & 0xFF;
    buffer_[57] = (bitLength >> 48) & 0xFF;
    buffer_[58] = (bitLength >> 40) & 0xFF;
    buffer_[59] = (bitLength >> 32) & 0xFF;
    buffer_[60] = (bitLength >> 24) & 0xFF;
    buffer_[61] = (bitLength >> 16) & 0xFF;
    buffer_[62] = (bitLength >> 8) & 0xFF;
    buffer_[63] = bitLength & 0xFF;

    // 处理最后一个块
    transform(buffer_);

    finalized_ = true;
  }

  void SHA1::transform(const uint8_t block[64])
  {
    uint32_t w[80];
    for (int i = 0; i < 16; ++i)
      w[i] = (block[i * 4] << 24) | (block[i * 4 + 1] << 16) | (block[i * 4 + 2] << 8) | block[i * 4 + 3];
    for (int i = 16; i < 80; ++i)
      w[i] = leftRotate(w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16], 1);

    uint32_t a = state_[0], b = state_[1], c = state_[2], d = state_[3], e = state_[4];

    for (int i = 0; i < 80; ++i)
    {
      uint32_t f, k;
      if (i < 20)
      {
        f = (b & c) | ((~b) & d);
        k = 0x5A827999;
      }
      else if (i < 40)
      {
        f = b ^ c ^ d;
        k = 0x6ED9EBA1;
      }
      else if (i < 60)
      {
        f = (b & c) | (b & d) | (c & d);
        k = 0x8F1BBCDC;
      }
      else
      {
        f = b ^ c ^ d;
        k = 0xCA62C1D6;
      }

      const uint32_t temp = leftRotate(a, 5) + f + e + k + w[i];
      e = d;
      d = c;
      c = leftRotate(b, 30);
      b = a;
      a = temp;
    }

    state_[0] += a;
    state_[1] += b;
    state_[2] += c;
    state_[3] += d;
    state_[4] += e;
  }

  uint32_t SHA1::leftRotate(const uint32_t x, const uint32_t n)
  {
    return (x << n) | (x >> (32 - n));
  }
}