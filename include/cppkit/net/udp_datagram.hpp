#pragma once

#include "net.hpp"
#include <sys/socket.h>
#include <string>
#include <netdb.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unordered_map>
#include <unistd.h>
#include <cstring>

namespace cppkit::net
{
  class UdpDatagram
  {
  public:
    UdpDatagram() = default;

    ~UdpDatagram() noexcept;

    // 禁用拷贝构造
    UdpDatagram(const UdpDatagram&) = delete;

    // 禁用拷贝赋值
    UdpDatagram& operator=(const UdpDatagram&) = delete;

    // 支持移动构造
    UdpDatagram(UdpDatagram&& other) noexcept;

    // 支持移动赋值
    UdpDatagram& operator=(UdpDatagram&& other) noexcept;

    // 绑定到指定的主机和端口
    // host 参数指定主机地址，port 参数指定端口号
    [[nodiscard]] bool bind(const std::string& host, uint16_t port);

    // 发送数据到指定的主机和端口
    // host 参数指定目标主机地址，port 参数指定目标端口号
    // data 参数指向要发送的数据，size 参数指定数据大小
    // 返回实际发送的字节数，失败时返回 -1
    ssize_t sendTo(const std::string& host, uint16_t port, char const* data, size_t size);

    // 从套接字接收数据
    // buffer 参数指向接收数据的缓冲区，size 参数指定缓冲
    // addr 参数可选，指向接收数据的源地址信息
    // 返回实际接收的字节数，失败时返回 -1
    ssize_t recvFrom(void* buffer, size_t size, sockaddr_in* addr = nullptr);

  private:
    // 检查套接字是否有效
    [[nodiscard]] bool isValid() const noexcept;

    // 初始化套接字，创建一个新的套接字描述符
    [[nodiscard]] bool ready() noexcept;

    int fd_{-1}; // 套接字文件描述符

    bool bind_{false}; // 是否已绑定

    std::unordered_map<std::string, sockaddr_storage> addrCache_; // 地址缓存
  };
}