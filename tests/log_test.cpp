#include "cppkit/log/log.hpp"
#include <array>

int main()
{
  CK_LOG_SET_LEVEL(cppkit::log::Level::Info);
  CK_LOG_SET_STDOUT(true);
  CK_LOG_SET_ROTATION(cppkit::log::Rotation::Daily);
  CK_LOG_SET_FILENAME_PATTERN("{year}-{month}/access-{date}.log");

  CK_LOG_INFO("program started...");

  std::array<int, 5> arr;
  return 0;
}