#include "cppkit/log/log.hpp"

int main()
{
  CK_LOG_INIT_FILE("access.log");
  CK_LOG_SET_LEVEL(cppkit::log::Level::Info);
  cppkit::log::Logger::instance().setToStdout(true);

  CK_LOG_INFO("This is an info message: %d", 42);
  CK_LOG_DEBUG("This is a debug message: %s", "debugging");
  CK_LOG_ERROR("This is an error message");
  return 0;
}