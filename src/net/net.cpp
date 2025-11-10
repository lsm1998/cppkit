#include "cppkit/net/net.hpp"

#include <stdexcept>
#include <cstring>
#include <netdb.h>

namespace cppkit::net
{
  AddrInfoResult resolveHost(const std::string& host,
      const uint16_t port,
      const int family,
      const int sockType,
      const bool passive)
  {
    addrinfo hints{};
    hints.ai_family = family;
    hints.ai_socktype = sockType;
    hints.ai_flags = passive ? AI_PASSIVE : 0;

    addrinfo* res = nullptr;

    if (const int ret = getaddrinfo(host.empty() ? nullptr : host.c_str(), std::to_string(port).c_str(), &hints, &res);
      ret != 0 || !res)
      throw std::runtime_error(std::string("gettarinfo failed: ") + gai_strerror(ret));

    return AddrInfoResult{res};
  }
}