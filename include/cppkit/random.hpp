#pragma once

#include <random>
#include <mutex>

namespace cppkit
{
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
  };
}