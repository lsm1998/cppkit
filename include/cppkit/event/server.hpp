#pragma once

#include "ae.hpp"
#include <cstdint>
#include <functional>
#include <mutex>
#include <string>
#include <vector>

namespace cppkit::event
{
  class ConnInfo
  {
    std::string ip;
    uint16_t port{};
    int fd{};
    std::function<void(int, int)> cleanup;

  public:
    ConnInfo() = default;

    ConnInfo(std::string ip, const uint16_t port, const int fd, const std::function<void(int, int)>& cleanup)
      : ip(std::move(ip)), port(port), fd(fd), cleanup(cleanup)
    {
    }

    // 获取IP地址
    [[nodiscard]]
    std::string getIp() const;

    // 获取端口号
    [[nodiscard]]
    uint16_t getPort() const;

    // 获取客户端标识
    [[nodiscard]]
    std::string getClientId() const;

    // 发送数据
    ssize_t send(const uint8_t* data, size_t length) const;

    // 接收数据
    ssize_t recv(uint8_t* data, size_t length) const;

    // 获取文件描述符
    [[nodiscard]]
    int getFd() const;

    // 关闭连接
    void close() const;

    // 是否同一个连接
    bool operator==(const ConnInfo& other) const noexcept;
  };

  extern std::unordered_map<int, ConnInfo> connections_;
  extern std::mutex connections_mutex_;

  class TcpServer
  {
  public:
    using OnConnection = std::function<void(const ConnInfo& conn)>;
    using OnMessage = std::function<void(const ConnInfo& conn, const std::vector<uint8_t>&)>;
    using OnClose = std::function<void(const ConnInfo& conn)>;
    using OnReadable = std::function<ssize_t(const ConnInfo& conn)>;

    TcpServer(EventLoop* loop, std::string addr, uint16_t port);


    TcpServer() = default;

    ~TcpServer();

    // 启动服务器
    void start();

    // 停止服务器
    void stop();

    // 设置连接回调函数
    void setOnConnection(OnConnection cb) { onConn_ = std::move(cb); }

    // 设置消息回调函数
    void setOnMessage(OnMessage cb) { onMsg_ = std::move(cb); }

    // 设置关闭连接回调函数
    void setOnClose(OnClose cb) { onClose_ = std::move(cb); }

    // 设置可读回调函数
    void setReadable(OnReadable cb) { onReadable_ = std::move(cb); }

    // 设置监听地址
    void setAddr(const std::string& addr) { addr_ = addr; }

    // 获取监听地址
    [[nodiscard]]
    std::string getAddr() const { return addr_; }

    // 设置监听端口
    void setPort(const uint16_t port) { port_ = port; }

    // 获取监听端口
    [[nodiscard]]
    uint16_t getPort() const { return port_; }

  private:
    // 清理连接
    void cleanup(ssize_t n, int cfd) const;

    EventLoop* loop_{}; // 事件循环指针

    int listen_fd_ = -1; // 监听套接字文件描述符

    std::string addr_; // 监听地址

    uint16_t port_{}; // 监听端口

    OnConnection onConn_; // 连接回调函数

    OnMessage onMsg_; // 消息回调函数

    OnClose onClose_; // 关闭连接回调函数

    OnReadable onReadable_; // 可读回调函数
  };
} // namespace cppkit::event