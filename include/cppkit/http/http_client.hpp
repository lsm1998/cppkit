#pragma once

#include "http_request.hpp"
#include <arpa/inet.h>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <map>
#include <netdb.h>
#include <string>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>

namespace cppkit::http {

class HttpClient {
private:
  // 超时时间
  size_t timeoutSeconds = 30;

public:
  HttpClient() = default;

  HttpClient(const HttpClient &) = delete;
  HttpClient &operator=(const HttpClient &) = delete;

  HttpClient(size_t timeoutSeconds) : timeoutSeconds(timeoutSeconds) {};

  ~HttpClient() = default;

public:
  HttpResponse Do(const HttpRequest &request);

  HttpResponse Get(const std::string &url,
                   const std::map<std::string, std::string> &headers = {});

  HttpResponse Post(const std::string &url,
                    const std::map<std::string, std::string> &headers = {},
                    const std::vector<uint8_t> &body = {});

  HttpResponse Delete(const std::string &url,
                      const std::map<std::string, std::string> &headers = {},
                      const std::vector<uint8_t> &body = {});

  HttpResponse Put(const std::string &url,
                   const std::map<std::string, std::string> &headers = {},
                   const std::vector<uint8_t> &body = {});

private:
  static void parseUrl(const std::string &url, std::string &host,
                       std::string &path, int &port, bool https);

  static int connect2host(const std::string &host, int port);

  static size_t sendData(int fd, std::vector<uint8_t> body);

  static std::vector<uint8_t> recvData(int fd);
};
} // namespace cppkit::http