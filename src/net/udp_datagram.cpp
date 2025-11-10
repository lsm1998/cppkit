#include "cppkit/net/udp_datagram.hpp"

namespace cppkit::net
{
  UdpDatagram::~UdpDatagram() noexcept
  {
    if (isValid())
    {
      close(fd_);
      fd_ = -1;
    }
  }

  UdpDatagram::UdpDatagram(UdpDatagram&& other) noexcept
  {
    this->fd_ = other.fd_;
    this->bind_ = other.bind_;
    this->addrCache_ = std::move(other.addrCache_);
    other.fd_ = -1;
    other.bind_ = false;
  }

  UdpDatagram& UdpDatagram::operator=(UdpDatagram&& other) noexcept
  {
    if (this != &other)
    {
      if (isValid())
        close(fd_);
      this->fd_ = other.fd_;
      this->bind_ = other.bind_;
      this->addrCache_ = std::move(other.addrCache_);
      other.fd_ = -1;
      other.bind_ = false;
    }
    return *this;
  }

  bool UdpDatagram::bind(const std::string& host, const uint16_t port)
  {
    if (!ready())
      return false;
    auto [res] = resolveHost(host, port, AF_INET, SOCK_DGRAM);
    bind_ = ::bind(fd_, res->ai_addr, res->ai_addrlen) == 0;
    return bind_;
  }

  ssize_t UdpDatagram::sendTo(const std::string& host, const uint16_t port, char const* data, const size_t size)
  {
    if (!ready())
      return -1;

    const std::string key = host + ":" + std::to_string(port);
    sockaddr_storage addr{};
    socklen_t addrLen = sizeof(addr);

    if (const auto it = addrCache_.find(key); it != addrCache_.end())
    {
      addr = it->second;
    }
    else
    {
      auto [res] = resolveHost(host, port, AF_INET, SOCK_DGRAM);
      addrLen = res->ai_addrlen;
      std::memcpy(&addr, res->ai_addr, addrLen);
      addrCache_[key] = addr;
    }

    return sendto(fd_, data, size, 0, reinterpret_cast<sockaddr*>(&addr), addrLen);
  }

  ssize_t UdpDatagram::recvFrom(void* buffer, const size_t size, sockaddr_in* addr)
  {
    if (!ready() || !bind_)
      return -1;
    socklen_t addrLen = sizeof(sockaddr_in);
    return recvfrom(fd_,
        buffer,
        size,
        0,
        reinterpret_cast<sockaddr*>(addr),
        &addrLen);
  }

  bool UdpDatagram::isValid() const noexcept
  {
    return fd_ > -1;
  }

  bool UdpDatagram::ready() noexcept
  {
    if (isValid())
      return true;
    fd_ = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd_ < 0)
      return false;

    // 设置 close-on-exec
    if (const int flags = fcntl(fd_, F_GETFD); flags != -1)
      fcntl(fd_, F_SETFD, flags | FD_CLOEXEC);

    return isValid();
  }
}