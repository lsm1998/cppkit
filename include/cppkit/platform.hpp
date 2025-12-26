#pragma once

namespace cppkit
{
#ifdef __APPLE__
    constexpr bool isApplePlatform = true;
#else
    constexpr bool isApplePlatform = false;
#endif

#ifdef __linux__
    constexpr bool isLinuxPlatform = true;
#else
    constexpr bool isLinuxPlatform = false;
#endif

#if defined(_WIN32) || defined(_WIN64)
    constexpr bool isWindowsPlatform = true;
#else
    constexpr bool isWindowsPlatform = false;
#endif

#ifdef __FreeBSD__
    constexpr bool isFreeBSDPlatform = true;
#else
    constexpr bool isFreeBSDPlatform = false;
#endif

#ifdef __ANDROID__
    constexpr bool isAndroidPlatform = true;
#else
    constexpr bool isAndroidPlatform = false;
#endif
}
