#pragma once

#include <string>
#include <unistd.h>
#include <chrono>

namespace cppkit::http
{
  class PoolConnection
  {
  public:
    int fd{}; // 文件描述符

    std::chrono::steady_clock::time_point lastUsed; // 上次使用时间

    std::string host; // 连接的主机地址
    
    int port{}; // 连接的端口号

    PoolConnection(const int fd, std::string host, const int port)
      : fd(fd)
        , lastUsed(std::chrono::steady_clock::now())
        , host(std::move(host))
        , port(port)
    {
    }

    PoolConnection(PoolConnection&& other) noexcept
      : fd(other.fd)
        , lastUsed(other.lastUsed)
        , host(std::move(other.host))
        , port(other.port)
    {
      other.fd = -1;
    }

    PoolConnection& operator=(PoolConnection&& other) noexcept
    {
      if (this != &other)
      {
        if (fd != -1)
        {
          close(fd);
        }
        fd = other.fd;
        lastUsed = other.lastUsed;
        host = std::move(other.host);
        port = other.port;
        other.fd = -1;
      }
      return *this;
    }

    PoolConnection(const PoolConnection&) = delete;
    PoolConnection& operator=(const PoolConnection&) = delete;

    ~PoolConnection()
    {
      if (fd != -1)
      {
        close(fd);
      }
    }
  };
}