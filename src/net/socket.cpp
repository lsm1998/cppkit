#include "cppkit/net/socket.hpp"

namespace cppkit::net
{
  Socket::Socket(Socket&& other) noexcept
  {
    fd_ = other.fd_;
    other.fd_ = -1;
  }

  Socket& Socket::operator=(Socket&& other) noexcept
  {
    if (this != &other)
    {
      if (isValid())
        ::close(fd_);
      fd_ = other.fd_;
      other.fd_ = -1;
    }
    return *this;
  }

  Socket::~Socket() noexcept
  {
    close();
  }

  void Socket::close() noexcept
  {
    if (isValid())
    {
      ::close(fd_);
      fd_ = -1;
    }
  }

  bool Socket::setNonBlocking(const bool blocking) const noexcept
  {
    if (!isValid())
      return false;

    // 获取当前文件描述符的标志
    const int flags = fcntl(fd_, F_GETFL, 0);
    if (flags == -1)
      return false;

    if (!blocking)
    {
      // 如果已经是非阻塞模式，直接返回 true
      if (flags & O_NONBLOCK)
        return true;
      return fcntl(fd_, F_SETFL, flags | O_NONBLOCK) >= 0;
    }
    // 如果已经是阻塞模式，直接返回 true
    if (!(flags & O_NONBLOCK))
      return true;
    return fcntl(fd_, F_SETFL, flags & ~O_NONBLOCK) >= 0;
  }

  bool Socket::setReuseAddr() const noexcept
  {
    if (!isValid())
      return false;
    constexpr int opt_val = 1;
    return setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &opt_val, sizeof(opt_val)) >= 0;
  }

  bool Socket::setReusePort() const noexcept
  {
    if (!isValid())
      return false;
    constexpr int opt_val = 1;
    return setsockopt(fd_, SOL_SOCKET, SO_REUSEPORT, &opt_val, sizeof(opt_val)) >= 0;
  }

  bool Socket::setKeepAlive(const bool enable) const noexcept
  {
    if (!isValid())
      return false;
    const int opt_val = enable ? 1 : 0;
    return setsockopt(fd_, SOL_SOCKET, SO_KEEPALIVE, &opt_val, sizeof(opt_val)) >= 0;
  }

  bool Socket::setNoDelay(const bool enable) const noexcept
  {
    if (!isValid())
      return false;
    const int opt_val = enable ? 1 : 0;
    return setsockopt(fd_, IPPROTO_TCP, TCP_NODELAY, &opt_val, sizeof(opt_val)) >= 0;
  }

  bool Socket::setBufferSize(const int size) const noexcept
  {
    return setSendBufferSize(size) && setReceiveBufferSize(size);
  }

  bool Socket::setSendBufferSize(const int size) const noexcept
  {
    if (!isValid())
      return false;
    return setsockopt(fd_, SOL_SOCKET, SO_SNDBUF, &size, sizeof(size)) >= 0;
  }

  bool Socket::setReceiveBufferSize(const int size) const noexcept
  {
    if (!isValid())
      return false;
    return setsockopt(fd_, SOL_SOCKET, SO_RCVBUF, &size, sizeof(size)) >= 0;
  }

  bool Socket::setTimeout(const int seconds) const noexcept
  {
    return setReceiveTimeout(seconds) && setSendTimeout(seconds);
  }

  bool Socket::setReceiveTimeout(const int seconds) const noexcept
  {
    if (!isValid())
      return false;
    timeval tv{};
    tv.tv_sec = seconds;
    tv.tv_usec = 0;
    return setsockopt(fd_, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) >= 0;
  }

  bool Socket::setSendTimeout(const int seconds) const noexcept
  {
    if (!isValid())
      return false;
    timeval tv{};
    tv.tv_sec = seconds;
    tv.tv_usec = 0;
    return setsockopt(fd_, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv)) >= 0;
  }

  bool Socket::shutdown(const int how) const noexcept
  {
    if (!isValid())
      return false;
    return ::shutdown(fd_, how) >= 0;
  }

  bool Socket::setOpt(const int level, const int optName, const void* optVal, const socklen_t optLen) const noexcept
  {
    if (!isValid())
      return false;
    return setsockopt(fd_, level, optName, optVal, optLen) >= 0;
  }

  bool Socket::setLinger(const bool enable, const int seconds) const noexcept
  {
    if (!isValid())
      return false;
    linger lin{};
    lin.l_onoff = enable ? 1 : 0;
    lin.l_linger = seconds;
    return setsockopt(fd_, SOL_SOCKET, SO_LINGER, &lin, sizeof(lin)) >= 0;
  }

  bool Socket::bind(const std::string& host, const uint16_t port) noexcept
  {
    if (!ready())
      return false;

    auto [res] = resolveHost(host, port, AF_INET,SOCK_STREAM, true);
    return ::bind(fd_, res->ai_addr, res->ai_addrlen) == 0;
  }

  bool Socket::connect(const std::string& host, const uint16_t port) noexcept
  {
    if (!ready())
      return false;

    auto [res] = resolveHost(host, port, AF_INET,SOCK_STREAM, true);
    return ::connect(fd_, res->ai_addr, res->ai_addrlen) == 0;
  }

  bool Socket::listen(const int length) const noexcept
  {
    if (!isValid())
      return false;
    return ::listen(fd_, length) >= 0;
  }

  Socket Socket::accept() const
  {
    if (!isValid())
      throw std::runtime_error("invalid socket");
    sockaddr_storage addr{};
    socklen_t addr_len = sizeof(addr);
    const auto client_fd = ::accept(fd_, reinterpret_cast<sockaddr*>(&addr), &addr_len);
    if (client_fd < 0)
      throw std::runtime_error("accept failed");
    return Socket(client_fd);
  }

  ssize_t Socket::write(const void* data, const size_t size, const int flags) const noexcept
  {
    return send(fd_, data, size, flags);
  }

  ssize_t Socket::read(void* data, const size_t size, const int flags) const noexcept
  {
    return recv(fd_, data, size, flags);
  }

  std::string Socket::getRemoteAddress() const noexcept
  {
    if (!isValid())
      return {};
    sockaddr_storage ss{};
    socklen_t len = sizeof(ss);
    if (getpeername(fd_, reinterpret_cast<sockaddr*>(&ss), &len) != 0)
      return {};
    char buf[INET6_ADDRSTRLEN] = {};
    if (ss.ss_family == AF_INET)
    {
      const auto* addr4 = reinterpret_cast<const sockaddr_in*>(&ss);
      if (inet_ntop(AF_INET, &addr4->sin_addr, buf, sizeof(buf)))
        return {buf};
    }
    else if (ss.ss_family == AF_INET6)
    {
      const auto* addr6 = reinterpret_cast<const sockaddr_in6*>(&ss);
      if (inet_ntop(AF_INET6, &addr6->sin6_addr, buf, sizeof(buf)))
        return {buf};
    }

    return {};
  }

  uint16_t Socket::getRemotePort() const noexcept
  {
    if (!isValid())
      return 0;
    sockaddr_storage ss{};
    socklen_t len = sizeof(ss);
    if (getpeername(fd_, reinterpret_cast<sockaddr*>(&ss), &len) != 0)
      return 0;

    if (ss.ss_family == AF_INET)
    {
      const auto* addr4 = reinterpret_cast<const sockaddr_in*>(&ss);
      return ntohs(addr4->sin_port);
    }
    if (ss.ss_family == AF_INET6)
    {
      const auto* addr6 = reinterpret_cast<const sockaddr_in6*>(&ss);
      return ntohs(addr6->sin6_port);
    }
    return 0;
  }

  bool Socket::isValid() const noexcept
  {
    return fd_ > -1;
  }

  bool Socket::ready() noexcept
  {
    if (isValid())
      return true;
    fd_ = ::socket(AF_INET, SOCK_STREAM, 0);
    if (fd_ < 0)
      return false;

    // 设置 close-on-exec
    if (const int flags = fcntl(fd_, F_GETFD); flags != -1)
      fcntl(fd_, F_SETFD, flags | FD_CLOEXEC);

    return isValid();
  }
} // namespace cppkit::net