#include "cppkit/crypto/sha512.hpp"

namespace cppkit::crypto
{
  void SHA512::reset()
  {
    state_ = {0x6a09e667f3bcc908ULL,
              0xbb67ae8584caa73bULL,
              0x3c6ef372fe94f82bULL,
              0xa54ff53a5f1d36f1ULL,
              0x510e527fade682d1ULL,
              0x9b05688c2b3e6c1fULL,
              0x1f83d9abfb41bd6bULL,
              0x5be0cd19137e2179ULL};
    bufferLen_ = 0;
    bitLen_ = 0;
    finalized_ = false;
    buffer_.fill(0);
  }

  void SHA512::update(const uint8_t* data, const size_t len)
  {
    size_t i = 0;
    while (i < len)
    {
      size_t toCopy = std::min(len - i, static_cast<size_t>(128 - bufferLen_));
      std::memcpy(buffer_.data() + bufferLen_, data + i, toCopy);
      bufferLen_ += toCopy;
      i += toCopy;

      if (bufferLen_ == 128)
      {
        transform(buffer_.data());
        bitLen_ += 1024;
        bufferLen_ = 0;
      }
    }
  }

  void SHA512::update(const std::string& str)
  {
    update(reinterpret_cast<const uint8_t*>(str.data()), str.size());
  }

  void SHA512::finalize()
  {
    if (finalized_)
      return;
    const uint64_t totalBits = bitLen_ + bufferLen_ * 8;

    const size_t padLen = (bufferLen_ < 112) ? (112 - bufferLen_) : (240 - bufferLen_);
    std::array<uint8_t, 256> pad{};
    pad[0] = 0x80;
    const uint64_t lowBits = totalBits;
    for (int i = 0; i < 8; ++i)
    {
      constexpr uint64_t highBits = 0;
      pad[padLen + i] = (highBits >> (56 - 8 * i)) & 0xFF;
      pad[padLen + 8 + i] = (lowBits >> (56 - 8 * i)) & 0xFF;
    }
    update(pad.data(), padLen + 16);

    finalized_ = true;
  }

  void SHA512::transform(const uint8_t block[128])
  {
    uint64_t w[80];
    for (int i = 0; i < 16; ++i)
    {
      w[i] = static_cast<uint64_t>(block[i * 8]) << 56 | static_cast<uint64_t>(block[i * 8 + 1]) << 48 |
             static_cast<uint64_t>(block[i * 8 + 2]) << 40 | static_cast<uint64_t>(block[i * 8 + 3]) << 32 |
             static_cast<uint64_t>(block[i * 8 + 4]) << 24 | static_cast<uint64_t>(block[i * 8 + 5]) << 16 |
             static_cast<uint64_t>(block[i * 8 + 6]) << 8 | static_cast<uint64_t>(block[i * 8 + 7]);
    }
    for (int i = 16; i < 80; ++i)
    {
      const uint64_t s0 = rotr(w[i - 15], 1) ^ rotr(w[i - 15], 8) ^ (w[i - 15] >> 7);
      const uint64_t s1 = rotr(w[i - 2], 19) ^ rotr(w[i - 2], 61) ^ (w[i - 2] >> 6);
      w[i] = w[i - 16] + s0 + w[i - 7] + s1;
    }

    uint64_t a = state_[0], b = state_[1], c = state_[2], d = state_[3];
    uint64_t e = state_[4], f = state_[5], g = state_[6], h = state_[7];

    for (int i = 0; i < 80; ++i)
    {
      const uint64_t S1 = rotr(e, 14) ^ rotr(e, 18) ^ rotr(e, 41);
      const uint64_t ch = (e & f) ^ ((~e) & g);
      const uint64_t temp1 = h + S1 + ch + K[i] + w[i];
      const uint64_t S0 = rotr(a, 28) ^ rotr(a, 34) ^ rotr(a, 39);
      const uint64_t maj = (a & b) ^ (a & c) ^ (b & c);
      const uint64_t temp2 = S0 + maj;

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

  uint64_t SHA512::rotr(const uint64_t x, const uint64_t n)
  {
    return x >> n | (x << (64 - n));
  }

  std::array<uint8_t, 64> SHA512::digest()
  {
    finalize();
    std::array<uint8_t, 64> out{};
    for (int i = 0; i < 8; ++i)
    {
      out[i * 8 + 0] = (state_[i] >> 56) & 0xFF;
      out[i * 8 + 1] = (state_[i] >> 48) & 0xFF;
      out[i * 8 + 2] = (state_[i] >> 40) & 0xFF;
      out[i * 8 + 3] = (state_[i] >> 32) & 0xFF;
      out[i * 8 + 4] = (state_[i] >> 24) & 0xFF;
      out[i * 8 + 5] = (state_[i] >> 16) & 0xFF;
      out[i * 8 + 6] = (state_[i] >> 8) & 0xFF;
      out[i * 8 + 7] = state_[i] & 0xFF;
    }
    return out;
  }

  std::string SHA512::hexDigest()
  {
    const auto d = digest();
    std::ostringstream oss;
    for (const auto b : d)
      oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(b);
    return oss.str();
  }
}