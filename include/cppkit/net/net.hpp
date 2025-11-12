#pragma once

#include <netinet/in.h>
#include <string>
#include <netdb.h>

namespace cppkit::net
{
  class Socket;

  class TcpServer;

  class TcpClient;

  class UdpDatagram;

  class WebSocket;

  struct AddrInfoResult
  {
    addrinfo* res{nullptr};

    ~AddrInfoResult()
    {
      if (res)
        freeaddrinfo(res);
    }
  };

  AddrInfoResult resolveHost(const std::string& host,
                                 uint16_t port,
                                 int family = AF_INET,
                                 int sockType = SOCK_STREAM,
                                 bool passive = false);
} // namespace cppkit::net