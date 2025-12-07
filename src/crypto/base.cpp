#include "cppkit/crypto/base.hpp"

namespace cppkit::crypto
{
  static constexpr char b64_table[] =
      "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

  static constexpr int b64_index[256] = {
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 62, -1, -1, -1, 63,
      52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -1, -1, -1, -1, -1, -1,
      -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14,
      15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, -1,
      -1, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
      41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51
  };

  std::string Base64::encode(const std::vector<uint8_t>& data)
  {
    std::string out;
    out.reserve(((data.size() + 2) / 3) * 4);

    size_t i = 0;
    while (i + 3 <= data.size())
    {
      const uint32_t n = (data[i] << 16) | (data[i + 1] << 8) | data[i + 2];
      out.push_back(b64_table[(n >> 18) & 63]);
      out.push_back(b64_table[(n >> 12) & 63]);
      out.push_back(b64_table[(n >> 6) & 63]);
      out.push_back(b64_table[n & 63]);
      i += 3;
    }

    // 2. 处理剩余的 1 字节或 2 字节 (填充逻辑)
    if (const size_t remainder = data.size() - i; remainder == 1)
    {
      // 剩余 1 字节：输出 XX==
      const uint32_t n = (data[i] << 16);
      out.push_back(b64_table[(n >> 18) & 63]);
      out.push_back(b64_table[(n >> 12) & 63]);
      out.push_back('='); // 填充第三位
      out.push_back('='); // 填充第四位
    }
    else if (remainder == 2)
    {
      // 剩余 2 字节：输出 XXX=
      const uint32_t n = (data[i] << 16) | (data[i + 1] << 8);
      out.push_back(b64_table[(n >> 18) & 63]);
      out.push_back(b64_table[(n >> 12) & 63]);
      out.push_back(b64_table[(n >> 6) & 63]);
      out.push_back('='); // 填充第四位
    }
    return out;
  }

  std::string Base64::encode(const std::string& data)
  {
    return encode(std::vector<uint8_t>(data.begin(), data.end()));
  }

  std::vector<uint8_t> Base64::decode(const std::string& input)
  {
    if (input.size() % 4 != 0)
      throw std::runtime_error("Invalid Base64 input length");

    size_t pad = 0;
    if (!input.empty() && input[input.size() - 1] == '=')
      pad++;
    if (input.size() > 1 && input[input.size() - 2] == '=')
      pad++;

    std::vector<uint8_t> out;
    out.reserve(input.size() / 4 * 3 - pad);

    for (size_t i = 0; i < input.size(); i += 4)
    {
      uint32_t n = 0;
      for (int j = 0; j < 4; ++j)
      {
        if (input[i + j] == '=')
          n <<= 6;
        else
        {
          const int val = b64_index[static_cast<unsigned char>(input[i + j])];
          if (val == -1)
            throw std::runtime_error("Invalid Base64 character");
          n = (n << 6) | val;
        }
      }

      out.push_back((n >> 16) & 0xFF);
      if (input[i + 2] != '=')
        out.push_back((n >> 8) & 0xFF);
      if (input[i + 3] != '=')
        out.push_back(n & 0xFF);
    }

    return out;
  }
} // namespace cppkit::crypto