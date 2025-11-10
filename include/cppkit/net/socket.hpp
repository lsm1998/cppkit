#pragma once

#include "net.hpp"
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <unistd.h>
#include <string>
#include <sys/socket.h>
#include <fcntl.h>

namespace cppkit::net
{
  class Socket
  {
  public:
    Socket() noexcept = default;

    explicit Socket(const int fd) noexcept : fd_(fd)
    {
    }

    // 禁用拷贝构造和拷贝赋值
    Socket(const Socket&) = delete;

    // 禁用拷贝构造和拷贝赋值
    Socket& operator=(const Socket&) = delete;

    // 启用移动构造和移动赋值
    Socket(Socket&& other) noexcept;

    // 启用移动构造和移动赋值
    Socket& operator=(Socket&& other) noexcept;

    ~Socket() noexcept;

    void close() noexcept;

    // 设置阻塞/非阻塞模式
    // 非阻塞模式下，I/O 操作会立即返回，不会阻塞等待数据
    [[nodiscard]] bool setNonBlocking(bool blocking = false) const noexcept;

    // 设置地址重用选项
    // 启用后，允许多个套接字绑定到同一地址和端口
    [[nodiscard]] bool setReuseAddr() const noexcept;

    // 设置端口重用选项
    // 允许多个套接字绑定到同一端口
    [[nodiscard]] bool setReusePort() const noexcept;

    // 设置保持连接选项
    // 启用后，TCP 会定期发送探测报文以保持连接活跃
    [[nodiscard]] bool setKeepAlive(bool enable = true) const noexcept;

    // 设置 TCP_NODELAY 选项
    // 启用后，禁用 Nagle 算法，减少延迟
    [[nodiscard]] bool setNoDelay(bool enable = true) const noexcept;

    // 同时设置发送和接收缓冲区大小
    // size 参数指定缓冲区大小，单位字节
    [[nodiscard]] bool setBufferSize(int size) const noexcept;

    // 设置发送缓冲区大小
    // size 参数指定缓冲区大小，单位字节
    [[nodiscard]] bool setSendBufferSize(int size) const noexcept;

    // 设置接收缓冲区大小
    // size 参数指定缓冲区大小，单位字节
    [[nodiscard]] bool setReceiveBufferSize(int size) const noexcept;

    // 同时设置发送和接收超时时间
    // seconds 参数指定超时时间，单位秒
    [[nodiscard]] bool setTimeout(int seconds) const noexcept;

    // 设置接收超时时间
    // seconds 参数指定超时时间，单位秒
    [[nodiscard]] bool setReceiveTimeout(int seconds) const noexcept;

    // 设置发送超时时间
    // seconds 参数指定超时时间，单位秒
    [[nodiscard]] bool setSendTimeout(int seconds) const noexcept;

    // 关闭套接字的读写功能
    // how 参数指定关闭方式，默认关闭读写两端
    [[nodiscard]] bool shutdown(int how = SHUT_RDWR) const noexcept;

    // 通用的套接字选项设置函数
    // level 参数指定选项级别，optName 指定选项名称
    // optVal 指向选项值，optLen 指定选项值长度
    [[nodiscard]] bool setOpt(int level, int optName, const void* optVal, socklen_t optLen) const noexcept;

    // 设置延迟关闭选项
    // enable 参数启用或禁用该选项，seconds 指定延迟时间，单位秒
    [[nodiscard]] bool setLinger(bool enable = true, int seconds = 0) const noexcept;

    // 绑定套接字到指定地址和端口
    // host 参数指定主机地址，port 参数指定端口号
    [[nodiscard]] bool bind(const std::string& host = "localhost", uint16_t port = 80) noexcept;

    // 连接到指定的远程主机和端口
    // host 参数指定远程主机地址，port 参数指定端口号
    [[nodiscard]] bool connect(const std::string& host = "localhost", uint16_t port = 80) noexcept;

    // 监听传入连接请求
    // length 参数指定最大连接队列长度，默认值为 SOMAXCONN
    [[nodiscard]] bool listen(int length = SOMAXCONN) const noexcept;

    // 接受一个传入的连接请求，返回一个新的 Socket 对象表示该连接
    // 如果接受失败，抛出运行时错误异常
    [[nodiscard]] Socket accept() const;

    // 发送数据到套接字
    // data 参数指向要发送的数据，size 参数指定数据大小，flags 参数指定发送标志
    // 返回实际发送的字节数，失败时返回 -1
    ssize_t write(const void* data, size_t size, int flags = 0) const noexcept;

    // 从套接字接收数据
    // data 参数指向接收数据的缓冲区，size 参数指定缓冲区大小，flags 参数指定接收标志
    // 返回实际接收的字节数，失败时返回 -1
    ssize_t read(void* data, size_t size, int flags = 0) const noexcept;

    // 获取远程主机的 IP 地址，返回字符串形式的地址
    [[nodiscard]] std::string getRemoteAddress() const noexcept;

    // 获取远程主机的端口号
    [[nodiscard]] uint16_t getRemotePort() const noexcept;

  private:
    // 检查套接字是否有效
    [[nodiscard]] bool isValid() const noexcept;

    // 初始化套接字，创建一个新的套接字描述符
    [[nodiscard]] bool ready() noexcept;

    int fd_{-1}; // 套接字文件描述符
  };
} // namespace cppkit::net