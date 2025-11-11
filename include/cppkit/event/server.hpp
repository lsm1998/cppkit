#pragma once

#include "ae.hpp"
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace cppkit::event
{
  class ConnInfo
  {
    std::string ip;
    uint16_t port{};
    int fd{};

  public:
    ConnInfo() = default;

    ConnInfo(std::string ip, const uint16_t port, const int fd) : ip(std::move(ip)), port(port), fd(fd)
    {
    }

    [[nodiscard]] std::string getIp() const;

    [[nodiscard]] uint16_t getPort() const;

    [[nodiscard]] std::string getClientId() const;

    ssize_t send(const uint8_t* data, size_t length) const;

    ssize_t recv(uint8_t* data, size_t length) const;

    [[nodiscard]] int getFd() const;

    bool operator==(const ConnInfo& other) const noexcept;
  };

  extern std::unordered_map<int, ConnInfo> connections_;

  class TcpServer
  {
  public:
    using OnConnection = std::function<void(const ConnInfo& conn)>;
    using OnMessage = std::function<void(const ConnInfo& conn, const std::vector<uint8_t>&)>;
    using OnClose = std::function<void(const ConnInfo& conn)>;

    TcpServer(EventLoop* loop, std::string addr, uint16_t port);
    TcpServer() = default;
    ~TcpServer();

    void start();
    void stop();

    void setOnConnection(OnConnection cb) { onConn_ = std::move(cb); }
    void setOnMessage(OnMessage cb) { onMsg_ = std::move(cb); }
    void setOnClose(OnClose cb) { onClose_ = std::move(cb); }

    void setAddr(const std::string& addr) { addr_ = addr; }
    [[nodiscard]] std::string getAddr() const { return addr_; }

    void setPort(const uint16_t port) { port_ = port; }
    [[nodiscard]] uint16_t getPort() const { return port_; }

  private:
    EventLoop* loop_;
    int listen_fd_ = -1;
    std::string addr_;
    uint16_t port_;
    OnConnection onConn_;
    OnMessage onMsg_;
    OnClose onClose_;
  };
} // namespace cppkit::event