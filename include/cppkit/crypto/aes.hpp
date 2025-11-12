#pragma once

#include <array>
#include <cstdint>
#include <vector>
#include <string>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <cstring>
#include <cassert>

namespace cppkit::crypto
{
  static constexpr int Nb = 4;  // block size (in 32-bit words): always 4 for AES
  static constexpr int Nk = 4;  // key length (in 32-bit words) for AES-128
  static constexpr int Nr = 10; // number of rounds for AES-128

  class AES
  {
  public:
    void test() {}
  };

  // AES S-box
  static const uint8_t sbox[256] = {0x63,
      0x7c,
      0x77,
      0x7b,
      0xf2,
      0x6b,
      0x6f,
      0xc5,
      0x30,
      0x01,
      0x67,
      0x2b,
      0xfe,
      0xd7,
      0xab,
      0x76,
      0xca,
      0x82,
      0xc9,
      0x7d,
      0xfa,
      0x59,
      0x47,
      0xf0,
      0xad,
      0xd4,
      0xa2,
      0xaf,
      0x9c,
      0xa4,
      0x72,
      0xc0,
      0xb7,
      0xfd,
      0x93,
      0x26,
      0x36,
      0x3f,
      0xf7,
      0xcc,
      0x34,
      0xa5,
      0xe5,
      0xf1,
      0x71,
      0xd8,
      0x31,
      0x15,
      0x04,
      0xc7,
      0x23,
      0xc3,
      0x18,
      0x96,
      0x05,
      0x9a,
      0x07,
      0x12,
      0x80,
      0xe2,
      0xeb,
      0x27,
      0xb2,
      0x75,
      0x09,
      0x83,
      0x2c,
      0x1a,
      0x1b,
      0x6e,
      0x5a,
      0xa0,
      0x52,
      0x3b,
      0xd6,
      0xb3,
      0x29,
      0xe3,
      0x2f,
      0x84,
      0x53,
      0xd1,
      0x00,
      0xed,
      0x20,
      0xfc,
      0xb1,
      0x5b,
      0x6a,
      0xcb,
      0xbe,
      0x39,
      0x4a,
      0x4c,
      0x58,
      0xcf,
      0xd0,
      0xef,
      0xaa,
      0xfb,
      0x43,
      0x4d,
      0x33,
      0x85,
      0x45,
      0xf9,
      0x02,
      0x7f,
      0x50,
      0x3c,
      0x9f,
      0xa8,
      0x51,
      0xa3,
      0x40,
      0x8f,
      0x92,
      0x9d,
      0x38,
      0xf5,
      0xbc,
      0xb6,
      0xda,
      0x21,
      0x10,
      0xff,
      0xf3,
      0xd2,
      0xcd,
      0x0c,
      0x13,
      0xec,
      0x5f,
      0x97,
      0x44,
      0x17,
      0xc4,
      0xa7,
      0x7e,
      0x3d,
      0x64,
      0x5d,
      0x19,
      0x73,
      0x60,
      0x81,
      0x4f,
      0xdc,
      0x22,
      0x2a,
      0x90,
      0x88,
      0x46,
      0xee,
      0xb8,
      0x14,
      0xde,
      0x5e,
      0x0b,
      0xdb,
      0xe0,
      0x32,
      0x3a,
      0x0a,
      0x49,
      0x06,
      0x24,
      0x5c,
      0xc2,
      0xd3,
      0xac,
      0x62,
      0x91,
      0x95,
      0xe4,
      0x79,
      0xe7,
      0xc8,
      0x37,
      0x6d,
      0x8d,
      0xd5,
      0x4e,
      0xa9,
      0x6c,
      0x56,
      0xf4,
      0xea,
      0x65,
      0x7a,
      0xae,
      0x08,
      0xba,
      0x78,
      0x25,
      0x2e,
      0x1c,
      0xa6,
      0xb4,
      0xc6,
      0xe8,
      0xdd,
      0x74,
      0x1f,
      0x4b,
      0xbd,
      0x8b,
      0x8a,
      0x70,
      0x3e,
      0xb5,
      0x66,
      0x48,
      0x03,
      0xf6,
      0x0e,
      0x61,
      0x35,
      0x57,
      0xb9,
      0x86,
      0xc1,
      0x1d,
      0x9e,
      0xe1,
      0xf8,
      0x98,
      0x11,
      0x69,
      0xd9,
      0x8e,
      0x94,
      0x9b,
      0x1e,
      0x87,
      0xe9,
      0xce,
      0x55,
      0x28,
      0xdf,
      0x8c,
      0xa1,
      0x89,
      0x0d,
      0xbf,
      0xe6,
      0x42,
      0x68,
      0x41,
      0x99,
      0x2d,
      0x0f,
      0xb0,
      0x54,
      0xbb,
      0x16};

  // Inverse S-box
  static const uint8_t inv_sbox[256] = {0x52,
      0x09,
      0x6A,
      0xD5,
      0x30,
      0x36,
      0xA5,
      0x38,
      0xBF,
      0x40,
      0xA3,
      0x9E,
      0x81,
      0xF3,
      0xD7,
      0xFB,
      0x7C,
      0xE3,
      0x39,
      0x82,
      0x9B,
      0x2F,
      0xFF,
      0x87,
      0x34,
      0x8E,
      0x43,
      0x44,
      0xC4,
      0xDE,
      0xE9,
      0xCB,
      0x54,
      0x7B,
      0x94,
      0x32,
      0xA6,
      0xC2,
      0x23,
      0x3D,
      0xEE,
      0x4C,
      0x95,
      0x0B,
      0x42,
      0xFA,
      0xC3,
      0x4E,
      0x08,
      0x2E,
      0xA1,
      0x66,
      0x28,
      0xD9,
      0x24,
      0xB2,
      0x76,
      0x5B,
      0xA2,
      0x49,
      0x6D,
      0x8B,
      0xD1,
      0x25,
      0x72,
      0xF8,
      0xF6,
      0x64,
      0x86,
      0x68,
      0x98,
      0x16,
      0xD4,
      0xA4,
      0x5C,
      0xCC,
      0x5D,
      0x65,
      0xB6,
      0x92,
      0x6C,
      0x70,
      0x48,
      0x50,
      0xFD,
      0xED,
      0xB9,
      0xDA,
      0x5E,
      0x15,
      0x46,
      0x57,
      0xA7,
      0x8D,
      0x9D,
      0x84,
      0x90,
      0xD8,
      0xAB,
      0x00,
      0x8C,
      0xBC,
      0xD3,
      0x0A,
      0xF7,
      0xE4,
      0x58,
      0x05,
      0xB8,
      0xB3,
      0x45,
      0x06,
      0xD0,
      0x2C,
      0x1E,
      0x8F,
      0xCA,
      0x3F,
      0x0F,
      0x02,
      0xC1,
      0xAF,
      0xBD,
      0x03,
      0x01,
      0x13,
      0x8A,
      0x6B,
      0x3A,
      0x91,
      0x11,
      0x41,
      0x4F,
      0x67,
      0xDC,
      0xEA,
      0x97,
      0xF2,
      0xCF,
      0xCE,
      0xF0,
      0xB4,
      0xE6,
      0x73,
      0x96,
      0xAC,
      0x74,
      0x22,
      0xE7,
      0xAD,
      0x35,
      0x85,
      0xE2,
      0xF9,
      0x37,
      0xE8,
      0x1C,
      0x75,
      0xDF,
      0x6E,
      0x47,
      0xF1,
      0x1A,
      0x71,
      0x1D,
      0x29,
      0xC5,
      0x89,
      0x6F,
      0xB7,
      0x62,
      0x0E,
      0xAA,
      0x18,
      0xBE,
      0x1B,
      0xFC,
      0x56,
      0x3E,
      0x4B,
      0xC6,
      0xD2,
      0x79,
      0x20,
      0x9A,
      0xDB,
      0xC0,
      0xFE,
      0x78,
      0xCD,
      0x5A,
      0xF4,
      0x1F,
      0xDD,
      0xA8,
      0x33,
      0x88,
      0x07,
      0xC7,
      0x31,
      0xB1,
      0x12,
      0x10,
      0x59,
      0x27,
      0x80,
      0xEC,
      0x5F,
      0x60,
      0x51,
      0x7F,
      0xA9,
      0x19,
      0xB5,
      0x4A,
      0x0D,
      0x2D,
      0xE5,
      0x7A,
      0x9F,
      0x93,
      0xC9,
      0x9C,
      0xEF,
      0xA0,
      0xE0,
      0x3B,
      0x4D,
      0xAE,
      0x2A,
      0xF5,
      0xB0,
      0xC8,
      0xEB,
      0xBB,
      0x3C,
      0x83,
      0x53,
      0x99,
      0x61,
      0x17,
      0x2B,
      0x04,
      0x7E,
      0xBA,
      0x77,
      0xD6,
      0x26,
      0xE1,
      0x69,
      0x14,
      0x63,
      0x55,
      0x21,
      0x0C,
      0x7D};

  // Round constant
  static const uint8_t Rcon[11] = {0x00, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x1B, 0x36};

  // Multiply in GF(2^8)
  static inline uint8_t xtime(uint8_t x)
  {
    return (uint8_t) ((x << 1) ^ ((x & 0x80) ? 0x1B : 0x00));
  }
  static inline uint8_t gmul(uint8_t a, uint8_t b)
  {
    uint8_t res = 0;
    while (b)
    {
      if (b & 1)
        res ^= a;
      a = xtime(a);
      b >>= 1;
    }
    return res;
  }

  // Key expansion: expand 16-uint8_t key into (Nb*(Nr+1)) * 4 uint8_ts = 176 uint8_ts for AES-128
  void KeyExpansion(const uint8_t key[16], uint8_t roundKey[176])
  {
    // first 16 uint8_ts are original key
    std::memcpy(roundKey, key, 16);
    int uint8_tsGenerated = 16;
    int rconIter = 1;
    uint8_t temp[4];

    while (uint8_tsGenerated < 4 * Nb * (Nr + 1))
    {
      // last 4 uint8_ts
      for (int i = 0; i < 4; ++i)
        temp[i] = roundKey[uint8_tsGenerated - 4 + i];

      if (uint8_tsGenerated % 16 == 0)
      {
        // rotate
        uint8_t t = temp[0];
        temp[0] = temp[1];
        temp[1] = temp[2];
        temp[2] = temp[3];
        temp[3] = t;
        // subuint8_ts
        temp[0] = sbox[temp[0]];
        temp[1] = sbox[temp[1]];
        temp[2] = sbox[temp[2]];
        temp[3] = sbox[temp[3]];
        // rcon
        temp[0] ^= Rcon[rconIter];
        rconIter++;
      }

      for (int i = 0; i < 4; ++i)
      {
        roundKey[uint8_tsGenerated] = roundKey[uint8_tsGenerated - 16] ^ temp[i];
        uint8_tsGenerated++;
      }
    }
  }

  // State is 4x4 uint8_ts column-major
  using State = std::array<std::array<uint8_t, 4>, 4>;

  static inline void Subuint8_ts(State& st)
  {
    for (int r = 0; r < 4; ++r)
      for (int c = 0; c < 4; ++c)
        st[c][r] = sbox[st[c][r]];
  }

  static inline void InvSubuint8_ts(State& st)
  {
    for (int r = 0; r < 4; ++r)
      for (int c = 0; c < 4; ++c)
        st[c][r] = inv_sbox[st[c][r]];
  }

  static inline void ShiftRows(State& st)
  {
    // row r rotated left by r
    for (int r = 1; r < 4; ++r)
    {
      uint8_t tmp[4];
      for (int c = 0; c < 4; ++c)
        tmp[c] = st[c][r];
      for (int c = 0; c < 4; ++c)
        st[c][r] = tmp[(c + r) % 4];
    }
  }

  static inline void InvShiftRows(State& st)
  {
    for (int r = 1; r < 4; ++r)
    {
      uint8_t tmp[4];
      for (int c = 0; c < 4; ++c)
        tmp[c] = st[c][r];
      for (int c = 0; c < 4; ++c)
        st[c][r] = tmp[(c - r + 4) % 4];
    }
  }

  static inline void MixColumns(State& st)
  {
    for (int c = 0; c < 4; ++c)
    {
      uint8_t a0 = st[c][0], a1 = st[c][1], a2 = st[c][2], a3 = st[c][3];
      st[c][0] = (uint8_t) (gmul(0x02, a0) ^ gmul(0x03, a1) ^ a2 ^ a3);
      st[c][1] = (uint8_t) (a0 ^ gmul(0x02, a1) ^ gmul(0x03, a2) ^ a3);
      st[c][2] = (uint8_t) (a0 ^ a1 ^ gmul(0x02, a2) ^ gmul(0x03, a3));
      st[c][3] = (uint8_t) (gmul(0x03, a0) ^ a1 ^ a2 ^ gmul(0x02, a3));
    }
  }

  static inline void InvMixColumns(State& st)
  {
    for (int c = 0; c < 4; ++c)
    {
      uint8_t a0 = st[c][0], a1 = st[c][1], a2 = st[c][2], a3 = st[c][3];
      st[c][0] = (uint8_t) (gmul(0x0e, a0) ^ gmul(0x0b, a1) ^ gmul(0x0d, a2) ^ gmul(0x09, a3));
      st[c][1] = (uint8_t) (gmul(0x09, a0) ^ gmul(0x0e, a1) ^ gmul(0x0b, a2) ^ gmul(0x0d, a3));
      st[c][2] = (uint8_t) (gmul(0x0d, a0) ^ gmul(0x09, a1) ^ gmul(0x0e, a2) ^ gmul(0x0b, a3));
      st[c][3] = (uint8_t) (gmul(0x0b, a0) ^ gmul(0x0d, a1) ^ gmul(0x09, a2) ^ gmul(0x0e, a3));
    }
  }

  static inline void AddRoundKey(State& st, const uint8_t* roundKey)
  {
    for (int c = 0; c < 4; ++c)
      for (int r = 0; r < 4; ++r)
        st[c][r] ^= roundKey[c * 4 + r];
  }

  void BlockToState(const uint8_t in[16], State& st)
  {
    for (int c = 0; c < 4; ++c)
      for (int r = 0; r < 4; ++r)
        st[c][r] = in[c * 4 + r];
  }

  void StateToBlock(const State& st, uint8_t out[16])
  {
    for (int c = 0; c < 4; ++c)
      for (int r = 0; r < 4; ++r)
        out[c * 4 + r] = st[c][r];
  }

  void AES_EncryptBlock(const uint8_t in[16], uint8_t out[16], const uint8_t roundKey[176])
  {
    State st;
    BlockToState(in, st);

    AddRoundKey(st, roundKey); // round 0

    for (int round = 1; round < Nr; ++round)
    {
      Subuint8_ts(st);
      ShiftRows(st);
      MixColumns(st);
      AddRoundKey(st, roundKey + round * 16);
    }

    // final round
    Subuint8_ts(st);
    ShiftRows(st);
    AddRoundKey(st, roundKey + Nr * 16);

    StateToBlock(st, out);
  }

  void AES_DecryptBlock(const uint8_t in[16], uint8_t out[16], const uint8_t roundKey[176])
  {
    State st;
    BlockToState(in, st);

    AddRoundKey(st, roundKey + Nr * 16);
    InvShiftRows(st);
    InvSubuint8_ts(st);

    for (int round = Nr - 1; round >= 1; --round)
    {
      AddRoundKey(st, roundKey + round * 16);
      InvMixColumns(st);
      InvShiftRows(st);
      InvSubuint8_ts(st);
    }

    AddRoundKey(st, roundKey); // round 0 key
    StateToBlock(st, out);
  }

  // PKCS#7 padding helpers
  std::vector<uint8_t> pkcs7_pad(const std::vector<uint8_t>& data)
  {
    size_t block = 16;
    size_t pad = block - (data.size() % block);
    std::vector<uint8_t> out = data;
    out.insert(out.end(), pad, (uint8_t) pad);
    return out;
  }

  std::vector<uint8_t> pkcs7_unpad(const std::vector<uint8_t>& data)
  {
    if (data.empty() || data.size() % 16 != 0)
      return {};
    uint8_t p = data.back();
    if (p == 0 || p > 16)
      return {}; // invalid
    for (size_t i = data.size() - p; i < data.size(); ++i)
      if (data[i] != p)
        return {}; // invalid
    std::vector<uint8_t> out(data.begin(), data.end() - p);
    return out;
  }

  // ECB mode (not recommended for real systems)
  std::vector<uint8_t> AES_Encrypt_ECB(const std::vector<uint8_t>& plaintext, const uint8_t key[16])
  {
    uint8_t roundKey[176];
    KeyExpansion(key, roundKey);
    std::vector<uint8_t> padded = pkcs7_pad(plaintext);
    std::vector<uint8_t> out(padded.size());
    uint8_t inblk[16], outblk[16];
    for (size_t i = 0; i < padded.size(); i += 16)
    {
      std::memcpy(inblk, padded.data() + i, 16);
      AES_EncryptBlock(inblk, outblk, roundKey);
      std::memcpy(out.data() + i, outblk, 16);
    }
    return out;
  }

  std::vector<uint8_t> AES_Decrypt_ECB(const std::vector<uint8_t>& ciphertext, const uint8_t key[16])
  {
    if (ciphertext.size() % 16 != 0)
      return {};
    uint8_t roundKey[176];
    KeyExpansion(key, roundKey);
    std::vector<uint8_t> out(ciphertext.size());
    uint8_t inblk[16], outblk[16];
    for (size_t i = 0; i < ciphertext.size(); i += 16)
    {
      std::memcpy(inblk, ciphertext.data() + i, 16);
      AES_DecryptBlock(inblk, outblk, roundKey);
      std::memcpy(out.data() + i, outblk, 16);
    }
    return pkcs7_unpad(out);
  }

  // CBC mode (requires iv of 16 uint8_ts)
  std::vector<uint8_t> AES_Encrypt_CBC(
      const std::vector<uint8_t>& plaintext, const uint8_t key[16], const uint8_t iv[16])
  {
    uint8_t roundKey[176];
    KeyExpansion(key, roundKey);
    std::vector<uint8_t> padded = pkcs7_pad(plaintext);
    std::vector<uint8_t> out(padded.size());
    uint8_t block[16], prev[16];
    std::memcpy(prev, iv, 16);
    for (size_t i = 0; i < padded.size(); i += 16)
    {
      for (int j = 0; j < 16; ++j)
        block[j] = padded[i + j] ^ prev[j];
      uint8_t enc[16];
      AES_EncryptBlock(block, enc, roundKey);
      std::memcpy(out.data() + i, enc, 16);
      std::memcpy(prev, enc, 16);
    }
    return out;
  }

  std::vector<uint8_t> AES_Decrypt_CBC(
      const std::vector<uint8_t>& ciphertext, const uint8_t key[16], const uint8_t iv[16])
  {
    if (ciphertext.size() % 16 != 0)
      return {};
    uint8_t roundKey[176];
    KeyExpansion(key, roundKey);
    std::vector<uint8_t> out(ciphertext.size());
    uint8_t prev[16], dec[16];
    std::memcpy(prev, iv, 16);
    for (size_t i = 0; i < ciphertext.size(); i += 16)
    {
      AES_DecryptBlock(ciphertext.data() + i, dec, roundKey);
      for (int j = 0; j < 16; ++j)
        out[i + j] = dec[j] ^ prev[j];
      std::memcpy(prev, ciphertext.data() + i, 16);
    }
    return pkcs7_unpad(out);
  }

  // Utilities
  std::string to_hex(const std::vector<uint8_t>& data)
  {
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (uint8_t b : data)
      oss << std::setw(2) << (int) b;
    return oss.str();
  }

  std::vector<uint8_t> from_string(const std::string& s)
  {
    return std::vector<uint8_t>(s.begin(), s.end());
  }

  // Example usage
  int Example(const std::string& key, const std::string& iv, const std::string& plain_text)
  {
    // // 16-uint8_t key (AES-128)
    // const uint8_t key[16] = {
    //     0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6, 0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c};
    // // IV for CBC (must be random in real use)
    // const uint8_t iv[16] = {
    //     0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};
    assert(key.size() == 16);
    assert(iv.size() == 16);
    const uint8_t* key_bytes = reinterpret_cast<const uint8_t*>(key.data());
    const uint8_t* iv_bytes = reinterpret_cast<const uint8_t*>(iv.data());

    std::vector<uint8_t> plaintext = from_string(plain_text);

    // Encrypt CBC
    std::vector<uint8_t> cipher = AES_Encrypt_CBC(plaintext, key_bytes, iv_bytes);
    std::cout << "加密后数据 (hex): " << to_hex(cipher) << "\n";

    // Decrypt CBC
    std::vector<uint8_t> decrypted = AES_Decrypt_CBC(cipher, key_bytes, iv_bytes);
    std::string recovered(decrypted.begin(), decrypted.end());
    std::cout << "加密前数据: " << recovered << "\n";

    // Also show ECB for single-block example
    std::string one = "Sixteen uint8_ts!!"; // exactly 16
    std::vector<uint8_t> onev(one.begin(), one.end());
    std::vector<uint8_t> ecb_enc = AES_Encrypt_ECB(onev, key_bytes);
    std::cout << "ECB Cipher (hex): " << to_hex(ecb_enc) << "\n";
    std::vector<uint8_t> ecb_dec = AES_Decrypt_ECB(ecb_enc, key_bytes);
    std::cout << "ECB Recovered: " << std::string(ecb_dec.begin(), ecb_dec.end()) << "\n";

    return 0;
  }

} // namespace cppkit::crypto