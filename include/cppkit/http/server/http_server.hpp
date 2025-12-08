#pragma once

#include "http_request.hpp"
#include "http_router.hpp"
#include "http_response.hpp"
#include "cppkit/event/server.hpp"
#include <string>
#include <functional>

namespace cppkit::http::server
{
  class HttpServer
  {
  public:
    explicit HttpServer(std::string host = "localhost", const int port = 80) : _port(port), _host(std::move(host))
    {
    }

    void start();

    void stop();

    void Get(const std::string& path, const HttpHandler& handler) const;

    void Post(const std::string& path, const HttpHandler& handler) const;

    void Put(const std::string& path, const HttpHandler& handler) const;

    void Delete(const std::string& path, const HttpHandler& handler) const;

    ~HttpServer() = default;

    void setPort(const int port) { _port = port; }

    [[nodiscard]] int getPort() const { return _port; }

    [[nodiscard]] std::string getHost() const { return _host; }

    void setHost(std::string host) { _host = std::move(host); }

  private:
    // 添加路由处理函数
    void addRoute(HttpMethod method, const std::string& path, const HttpHandler& handler) const;

    // 处理HTTP请求
    void handleRequest(const HttpRequest& request, HttpResponseWriter& writer) const;

    int _port;
    std::string _host;
    Router _router;
    event::EventLoop _loop{};
    event::TcpServer _server{};
  };
} // namespace cppkit::http