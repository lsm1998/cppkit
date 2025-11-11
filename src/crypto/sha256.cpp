#include "cppkit/crypto/sha256.hpp"

namespace cppkit::crypto
{
  static constexpr uint32_t K[64] = {0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b,
                                     0x59f111f1, 0x923f82a4, 0xab1c5ed5, 0xd807aa98, 0x12835b01,
                                     0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7,
                                     0xc19bf174, 0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
                                     0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da, 0x983e5152,
                                     0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147,
                                     0x06ca6351, 0x14292967, 0x27b70a85, 0x2e1b2138, 0x4d2c6dfc,
                                     0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
                                     0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819,
                                     0xd6990624, 0xf40e3585, 0x106aa070, 0x19a4c116, 0x1e376c08,
                                     0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f,
                                     0x682e6ff3, 0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
                                     0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2};

  void SHA256::update(const uint8_t* data, size_t len)
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

  void SHA256::update(const std::string& str) { update(reinterpret_cast<const uint8_t*>(str.data()), str.size()); }

  std::array<uint8_t, 32> SHA256::digest()
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

  std::string SHA256::hexDigest()
  {
    const auto d = digest();
    std::ostringstream oss;
    for (const auto b : d)
      oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(b);
    return oss.str();
  }

  void SHA256::reset()
  {
    state_[0] = 0x6a09e667;
    state_[1] = 0xbb67ae85;
    state_[2] = 0x3c6ef372;
    state_[3] = 0xa54ff53a;
    state_[4] = 0x510e527f;
    state_[5] = 0x9b05688c;
    state_[6] = 0x1f83d9ab;
    state_[7] = 0x5be0cd19;

    bufferLen_ = 0;
    totalBits_ = 0;
    finalized_ = false;
  }

  std::string SHA256::sha(const std::string& message)
  {
    SHA256 sha256;
    sha256.update(message);
    return sha256.hexDigest();
  }

  std::string SHA256::hmac(const std::string& key, const std::string& message)
  {
    constexpr size_t blockSize = 64;
    std::vector<uint8_t> keyBytes(key.begin(), key.end());

    if (keyBytes.size() > blockSize)
    {
      SHA256 sha256;
      sha256.update(keyBytes.data(), keyBytes.size());
      auto hashedKey = sha256.digest();
      keyBytes = std::vector<uint8_t>(hashedKey.begin(), hashedKey.end());
    }

    keyBytes.resize(blockSize, 0x00);

    std::vector<uint8_t> oKeyPad(blockSize);
    std::vector<uint8_t> iKeyPad(blockSize);
    for (size_t i = 0; i < blockSize; ++i)
    {
      oKeyPad[i] = keyBytes[i] ^ 0x5c;
      iKeyPad[i] = keyBytes[i] ^ 0x36;
    }

    SHA256 innerSha256;
    innerSha256.update(iKeyPad.data(), iKeyPad.size());
    innerSha256.update(reinterpret_cast<const uint8_t*>(message.data()), message.size());
    auto innerHash = innerSha256.digest();

    SHA256 outerSha256;
    outerSha256.update(oKeyPad.data(), oKeyPad.size());
    outerSha256.update(innerHash.data(), innerHash.size());
    auto hmacHash = outerSha256.digest();

    std::ostringstream oss;
    for (const auto b : hmacHash)
      oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(b);
    return oss.str();
  }

  void SHA256::finalize()
  {
    if (finalized_)
      return;
    totalBits_ += bufferLen_ * 8;

    uint8_t pad[64] = {0x80};
    size_t padLen = (bufferLen_ < 56) ? (56 - bufferLen_) : (120 - bufferLen_);
    update(pad, padLen);

    uint8_t lenBytes[8];
    for (int i = 0; i < 8; ++i)
      lenBytes[7 - i] = (totalBits_ >> (i * 8)) & 0xFF;
    update(lenBytes, 8);

    finalized_ = true;
  }

  void SHA256::transform(const uint8_t block[64])
  {
    uint32_t w[64];
    for (int i = 0; i < 16; i++)
      w[i] = (block[i * 4] << 24) | (block[i * 4 + 1] << 16) | (block[i * 4 + 2] << 8) | block[i * 4 + 3];
    for (int i = 16; i < 64; i++)
    {
      const uint32_t s0 = rotr(w[i - 15], 7) ^ rotr(w[i - 15], 18) ^ (w[i - 15] >> 3);
      const uint32_t s1 = rotr(w[i - 2], 17) ^ rotr(w[i - 2], 19) ^ (w[i - 2] >> 10);
      w[i] = w[i - 16] + s0 + w[i - 7] + s1;
    }

    uint32_t a = state_[0], b = state_[1], c = state_[2], d = state_[3];
    uint32_t e = state_[4], f = state_[5], g = state_[6], h = state_[7];

    for (int i = 0; i < 64; i++)
    {
      const uint32_t S1 = rotr(e, 6) ^ rotr(e, 11) ^ rotr(e, 25);
      const uint32_t ch = (e & f) ^ ((~e) & g);
      const uint32_t temp1 = h + S1 + ch + K[i] + w[i];
      const uint32_t S0 = rotr(a, 2) ^ rotr(a, 13) ^ rotr(a, 22);
      const uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
      const uint32_t temp2 = S0 + maj;

      h = g;
      g = f;
      f = e;
      e = d + temp1;
      d = c;
      c = b;
      b = a;
      a = temp1 + temp2;
    }

    state_[0] += a;
    state_[1] += b;
    state_[2] += c;
    state_[3] += d;
    state_[4] += e;
    state_[5] += f;
    state_[6] += g;
    state_[7] += h;
  }

  uint32_t SHA256::rotr(const uint32_t x, const uint32_t n)
  {
    return (x >> n) | (x << (32 - n));
  }
}