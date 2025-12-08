#include "cppkit/http/http_client.hpp"
#include "cppkit/define.hpp"
#include <netdb.h>
#include <stdexcept>
#include <unistd.h>
#include <vector>
#include <sys/select.h>
#include <fcntl.h>
#include <ranges>
#include <netinet/tcp.h>

namespace cppkit::http
{
  HttpResponse HttpClient::Get(const std::string& url, const std::map<std::string, std::string>& headers)
  {
    const HttpRequest request(HttpMethod::Get, url, headers);
    return this->Do(request);
  }

  HttpResponse HttpClient::Post(
      const std::string& url,
      const std::map<std::string, std::string>& headers,
      const std::vector<uint8_t>& body)
  {
    const HttpRequest request(HttpMethod::Post, url, headers, body);
    return this->Do(request);
  }

  HttpResponse HttpClient::Put(
      const std::string& url,
      const std::map<std::string, std::string>& headers,
      const std::vector<uint8_t>& body)
  {
    const HttpRequest request(HttpMethod::Put, url, headers, body);
    return this->Do(request);
  }

  HttpResponse HttpClient::Delete(
      const std::string& url,
      const std::map<std::string, std::string>& headers,
      const std::vector<uint8_t>& body)
  {
    const HttpRequest request(HttpMethod::Delete, url, headers, body);
    return this->Do(request);
  }

  HttpResponse HttpClient::Do(const HttpRequest& request)
  {
    // 解析url域名对应的ip地址
    std::string host, path;
    int port{};
    const bool https = request.url.starts_with("https");
    // 不支持https
    if (https)
    {
      throw std::runtime_error("HTTPS is not supported yet");
    }
    parseUrl(request.url, host, path, port, https);

    // 获取连接
    auto conn = getConnection(host, port);

    // 设置TCP_NODELAY选项，禁用Nagle算法
    constexpr int flag = 1;
    setsockopt(conn->fd, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(int));

    // 发送请求
    if (const auto requestData = request.build(host, path, port, https);
      sendData(conn->fd, requestData) <= 0)
    {
      conn->fd = -1; // 标记连接为关闭
      throw std::runtime_error("Failed to send request to " + host);
    }

    // 接受响应
    const auto data = recvData(conn->fd);

    if (data.empty())
    {
      conn->fd = -1; // 标记连接为关闭
      throw std::runtime_error("Empty response from " + host);
    }

    // 解析响应
    HttpResponse response = HttpResponse::parse(data);

    // 处理连接保持活动状态
    bool keepAlive = false;

    // 检查响应头中的Connection字段
    const std::string connHeader = response.getHeader("Connection");

    // 默认HTTP/1.1是保持连接的，除非明确指定关闭
    keepAlive = connHeader.empty() || connHeader == "keep-alive";

    if (keepAlive)
    {
      returnConnection(std::move(conn));
    }
    else
    {
      conn->fd = -1; // 标记连接为关闭
    }

    return response;
  }

  void HttpClient::parseUrl(const std::string& url, std::string& host, std::string& path, int& port, bool https)
  {
    const std::string prefix = https ? "https://" : "http://";
    const size_t start = prefix.size();
    const size_t slash = url.find('/', start);
    if (slash == std::string::npos)
    {
      host = url.substr(start);
      path = "/";
    }
    else
    {
      host = url.substr(start, slash - start);
      path = url.substr(slash);
    }

    // 是否存在端口
    if (const size_t colon = host.find(':'); colon != std::string::npos)
    {
      port = std::stoi(host.substr(colon + 1));
      host = host.substr(0, colon);
    }
    else
    {
      port = https ? 443 : 80;
    }

    in_addr addr4{};
    in6_addr addr6{};
    const bool is_ipv4 = inet_pton(AF_INET, host.c_str(), &addr4) == 1;
    const bool is_ipv6 = inet_pton(AF_INET6, host.c_str(), &addr6) == 1;

    if (is_ipv4 || is_ipv6)
    {
      // 已经是IP地址了
      return;
    }

    // 将host从域名转为ip地址
    addrinfo hints{}, *res = nullptr;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if (const int status = getaddrinfo(host.c_str(), nullptr, &hints, &res); status != 0 || !res)
    {
      throw std::runtime_error("DNS resolution failed for host: " + host + " (" + gai_strerror(status) + ")");
    }

    char ipstr[INET6_ADDRSTRLEN] = {0};
    const void* addr = nullptr;

    if (res->ai_family == AF_INET)
    {
      const auto* ipv4 = reinterpret_cast<struct sockaddr_in*>(res->ai_addr);
      addr = &ipv4->sin_addr;
    }
    else if (res->ai_family == AF_INET6)
    {
      const auto* ipv6 = reinterpret_cast<struct sockaddr_in6*>(res->ai_addr);
      addr = &ipv6->sin6_addr;
    }

    if (addr)
    {
      inet_ntop(res->ai_family, addr, ipstr, sizeof(ipstr));
      host = ipstr;
    }

    freeaddrinfo(res);
  }

  int HttpClient::connect2host(const std::string& host, const int port, const size_t timeoutSeconds)
  {
    addrinfo hints{}, *res;
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    const std::string port_str = std::to_string(port);
    if (getaddrinfo(host.c_str(), port_str.c_str(), &hints, &res) != 0)
    {
      return -1;
    }

    const int fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (fd < 0)
    {
      freeaddrinfo(res);
      return -1;
    }

    // Set non-blocking mode
    const int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1)
    {
      close(fd);
      freeaddrinfo(res);
      return -1;
    }

    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1)
    {
      close(fd);
      freeaddrinfo(res);
      return -1;
    }

    // Connect with timeout
    int connectResult = connect(fd, res->ai_addr, res->ai_addrlen);
    if (connectResult < 0)
    {
      if (errno != EINPROGRESS)
      {
        close(fd);
        freeaddrinfo(res);
        return -1;
      }

      fd_set writefds;
      FD_ZERO(&writefds);
      FD_SET(fd, &writefds);

      struct timeval tv;
      tv.tv_sec = timeoutSeconds;
      tv.tv_usec = 0;

      int selectResult = select(fd + 1, nullptr, &writefds, nullptr, &tv);
      if (selectResult <= 0)
      {
        close(fd);
        freeaddrinfo(res);
        errno = (selectResult == 0) ? ETIMEDOUT : errno;
        return -1;
      }

      // Check if connection succeeded
      int so_error;
      socklen_t len = sizeof(so_error);
      if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &so_error, &len) < 0 || so_error != 0)
      {
        close(fd);
        freeaddrinfo(res);
        errno = so_error;
        return -1;
      }
    }

    // Set back to blocking mode
    if (fcntl(fd, F_SETFL, flags) == -1)
    {
      close(fd);
      freeaddrinfo(res);
      return -1;
    }

    freeaddrinfo(res);
    return fd;
  }

  size_t HttpClient::sendData(const int fd, const std::vector<uint8_t>& body)
  {
    size_t totalSent = 0;
    const size_t size = body.size();

    while (totalSent < size)
    {
      ssize_t sent = ::send(fd, body.data() + totalSent, size - totalSent, 0);

      if (sent < 0)
      {
        if (errno == EINTR)
          continue;
        return -1;
      }

      if (sent == 0)
        break;

      totalSent += sent;
    }

    return totalSent;
  }

  std::vector<uint8_t> HttpClient::recvData(int fd)
  {
    std::vector<uint8_t> buffer;
    uint8_t data[DEFAULT_BUFFER_SIZE];

    while (true)
    {
      const ssize_t n = ::recv(fd, data, sizeof(data), 0);
      if (n <= 0)
        break;
      buffer.insert(buffer.end(), data, data + n);
    }

    return buffer;
  }

  HttpClient::~HttpClient()
  {
    // Close all connections
    std::unique_lock<std::mutex> lock(poolMutex);
    for (auto& pair : connectionPool)
    {
      auto& queue = pair.second;
      while (!queue.empty())
      {
        queue.pop(); // Connection will be closed by destructor
      }
    }
  }

  bool HttpClient::isConnectionAlive(int fd)
  {
    // Send a zero-byte message to check if connection is alive
    // This uses MSG_PEEK to avoid consuming any data
    char buffer[1];
    const ssize_t n = ::recv(fd, buffer, sizeof(buffer), MSG_PEEK | MSG_DONTWAIT);
    if (n == 0)
    {
      // Connection closed
      return false;
    }
    else if (n < 0)
    {
      // Check if it's a recoverable error
      if (errno != EAGAIN && errno != EWOULDBLOCK)
      {
        return false;
      }
    }
    return true;
  }

  std::unique_ptr<PoolConnection> HttpClient::getConnection(const std::string& host, int port)
  {
    std::unique_lock lock(poolMutex);

    // Clean up old connections
    cleanUpOldConnections();

    // Create pool key
    const std::string key = host + ":" + std::to_string(port);

    // Try to get a connection from pool
    while (true)
    {
      if (connectionPool.contains(key))
      {
        auto& queue = connectionPool[key];
        while (!queue.empty())
        {
          auto conn = std::move(queue.front());
          queue.pop();

          // Check if connection is still alive
          if (isConnectionAlive(conn->fd))
          {
            conn->lastUsed = std::chrono::steady_clock::now();
            return conn;
          }
          // Dead connection will be closed by destructor
        }
      }

      // Calculate total connections in pool
      size_t totalConnections = 0;
      for (const auto& val : connectionPool | std::views::values)
      {
        totalConnections += val.size();
      }

      // 如果连接数未达上限，创建新连接
      if (totalConnections < maxConnections)
      {
        lock.unlock();

        int fd = connect2host(host, port, connectionTimeout);
        if (fd < 0)
        {
          throw std::runtime_error("Failed to connect to " + host + ":" + std::to_string(port));
        }

        return std::make_unique<PoolConnection>(fd, host, port);
      }

      // 等待有连接可用
      poolCond.wait_for(lock, std::chrono::seconds(timeoutSeconds));
    }
  }

  void HttpClient::returnConnection(std::unique_ptr<PoolConnection> conn)
  {
    if (!conn || conn->fd == -1)
    {
      return;
    }

    std::unique_lock lock(poolMutex);

    // 构建缓存key
    const std::string key = conn->host + ":" + std::to_string(conn->port);

    // 更新最后使用时间
    conn->lastUsed = std::chrono::steady_clock::now();

    // 将连接放回连接池
    connectionPool[key].push(std::move(conn));

    // 通知等待的线程有连接可用
    poolCond.notify_one();
  }

  void HttpClient::cleanUpOldConnections()
  {
    const auto now = std::chrono::steady_clock::now();
    constexpr auto timeout = std::chrono::minutes(5);

    for (auto& val : connectionPool | std::views::values)
    {
      auto& queue = val;
      std::queue<std::unique_ptr<PoolConnection>> newQueue;

      while (!queue.empty())
      {
        auto conn = std::move(queue.front());
        queue.pop();

        if (std::chrono::duration_cast<std::chrono::minutes>(now - conn->lastUsed) < timeout)
        {
          newQueue.push(std::move(conn));
        }
        // Old connection will be closed by destructor
      }

      queue.swap(newQueue);
    }
  }
} // namespace cppkit::http