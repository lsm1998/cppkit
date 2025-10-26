#include "cppkit/http/http_client.hpp"
#include <cstdint>
#include <stdexcept>
#include <unistd.h>
#include <vector>

namespace cppkit::http {
HttpResponse
HttpClient::Get(const std::string &url,
                const std::map<std::string, std::string> &headers) {
  HttpRequest request(HttpMethod::Get, url, headers);
  return this->Do(request);
}

HttpResponse HttpClient::Post(const std::string &url,
                              const std::map<std::string, std::string> &headers,
                              const std::vector<uint8_t> &body) {
  HttpRequest request(HttpMethod::Post, url, headers, body);
  return this->Do(request);
}

HttpResponse HttpClient::Put(const std::string &url,
                             const std::map<std::string, std::string> &headers,
                             const std::vector<uint8_t> &body) {
  HttpRequest request(HttpMethod::Put, url, headers, body);
  return this->Do(request);
}

HttpResponse
HttpClient::Delete(const std::string &url,
                   const std::map<std::string, std::string> &headers,
                   const std::vector<uint8_t> &body) {
  HttpRequest request(HttpMethod::Delete, url, headers, body);
  return this->Do(request);
}

HttpResponse HttpClient::Do(const HttpRequest &request) {
  // 解析url域名对应的ip地址
  std::string host, path;
  int port{};
  bool https = request.url.starts_with("https");
  // 不支持https
  if (https) {
    throw std::runtime_error("HTTPS is not supported yet");
  }
  parseUrl(request.url, host, path, port, https);
  int fd = connect2host(host, port);

  if (sendData(fd, request.buildRequestData(host, path, port, https)) <= 0) {
    close(fd);
    throw std::runtime_error("Failed to send request to " + host);
  }

  // 读取响应
  auto data = recvData(fd);
  return HttpResponse::parseResponse(data);
}

void HttpClient::parseUrl(const std::string &url, std::string &host,
                          std::string &path, int &port, bool https) {
  std::string prefix = https ? "https://" : "http://";
  size_t start = prefix.size();
  size_t slash = url.find('/', start);
  host = url.substr(start, slash - start);
  path = (slash == std::string::npos) ? "/" : url.substr(slash);

  // 是否存在端口
  size_t colon = host.find(':');
  if (colon != std::string::npos) {
    port = std::stoi(host.substr(colon + 1));
    host = host.substr(0, colon);
  } else {
    port = https ? 443 : 80;
  }

  struct in_addr addr4;
  struct in6_addr addr6;
  bool is_ipv4 = inet_pton(AF_INET, host.c_str(), &addr4) == 1;
  bool is_ipv6 = inet_pton(AF_INET6, host.c_str(), &addr6) == 1;

  if (is_ipv4 || is_ipv6) { // 已经是IP地址了
    return;
  }

  // 将host从域名转为ip地址
  struct addrinfo hints{}, *res = nullptr;
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;

  int status = getaddrinfo(host.c_str(), nullptr, &hints, &res);
  if (status != 0 || !res) {
    throw std::runtime_error("DNS resolution failed for host: " + host + " (" +
                             gai_strerror(status) + ")");
  }

  char ipstr[INET6_ADDRSTRLEN] = {0};
  void *addr = nullptr;

  if (res->ai_family == AF_INET) {
    struct sockaddr_in *ipv4 =
        reinterpret_cast<struct sockaddr_in *>(res->ai_addr);
    addr = &(ipv4->sin_addr);
  } else if (res->ai_family == AF_INET6) {
    struct sockaddr_in6 *ipv6 =
        reinterpret_cast<struct sockaddr_in6 *>(res->ai_addr);
    addr = &(ipv6->sin6_addr);
  }

  if (addr) {
    inet_ntop(res->ai_family, addr, ipstr, sizeof(ipstr));
    host = ipstr;
  }

  freeaddrinfo(res);
}

int HttpClient::connect2host(const std::string &host, int port) {
  struct addrinfo hints{}, *res;
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;

  std::string port_str = std::to_string(port);
  if (getaddrinfo(host.c_str(), port_str.c_str(), &hints, &res) != 0) {
    return -1;
  }

  int fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
  if (fd < 0) {
    return -1;
  }

  if (connect(fd, res->ai_addr, res->ai_addrlen) < 0) {
    close(fd);
    freeaddrinfo(res);
    return -1;
  }

  freeaddrinfo(res);
  return fd;
}

size_t HttpClient::sendData(int fd, std::vector<uint8_t> body) {
  size_t totalSent = 0;
  size_t size = body.size();

  while (totalSent < size) {
    ssize_t sent = ::send(fd, body.data() + totalSent, size - totalSent, 0);

    if (sent < 0) {
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

std::vector<uint8_t> HttpClient::recvData(int fd) {
  std::vector<uint8_t> buffer;
  uint8_t data[BUFFER_SIZE];

  while (true) {
    int n = ::recv(fd, data, sizeof(data), 0);
    if (n <= 0)
      break;
    buffer.insert(buffer.end(), data, data + n);
  }

  return buffer;
}

} // namespace cppkit::http
