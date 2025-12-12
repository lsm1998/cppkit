#pragma once

#include <random>
#include <string>
#include <string_view>
#include <array>

namespace cppkit
{
  constexpr std::string_view lowerChars = "abcdefghijklmnopqrstuvwxyz";

  constexpr std::string_view upperChars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";

  constexpr std::string_view digitChars = "0123456789";

  constexpr std::string_view symbolChars = "!@#$%^&*()-_=+[]{}|;:',.<>/?`~ ";

  constexpr std::string_view allChars = "abcdefghijklmnopqrstuvwxyz"
                                       "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                       "0123456789"
                                       "!@#$%^&*()-_=+[]{}|;:',.<>/?`~ ";

  class Random
  {
  public:
    static int nextInt(const int max)
    {
      return nextInt(0, max);
    }

    static int nextInt(const int min, const int max)
    {
      thread_local std::mt19937 gen(std::random_device{}());
      std::uniform_int_distribution dist(min, max);
      return dist(gen);
    }

    static double nextDouble(const double min, const double max)
    {
      thread_local std::mt19937 gen(std::random_device{}());
      std::uniform_real_distribution dist(min, max);
      return dist(gen);
    }

    static std::string randomString(const int len, std::string_view charset = allChars)
    {
      thread_local std::mt19937 gen(std::random_device{}());
      std::uniform_int_distribution dist(0, static_cast<int>(charset.size() - 1));

      std::string result;
      result.reserve(len);
      for (int i = 0; i < len; ++i)
      {
        result += charset[dist(gen)];
      }
      return result;
    }
  };
}