#include "cppkit/log/log.hpp"
#include "cppkit/time.hpp"

int main()
{
    CK_LOG_SET_LEVEL(cppkit::log::Level::Info);
    CK_LOG_SET_STDOUT(true);
    CK_LOG_SET_ROTATION(cppkit::log::Rotation::Daily);
    CK_LOG_SET_FILENAME_PATTERN("{year}-{month}/access-{date}.log");

    CK_LOG_INFO("program started...");

    CK_LOG_INFO("level = {}", cppkit::Time::Now().Format());

    CK_LOG_FLUSH();
    return 0;
}
