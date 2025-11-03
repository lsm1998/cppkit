#pragma once

#include "http_request.hpp"
#include <string>
#include <functional>

namespace cppkit::http
{
  using HttpHandler = std::function<std::string(const HttpRequest& request, HttpResponse& response)>;

  class HttpServer
  {
  public:
    HttpServer(std::string host = "localhost", int port = 80) : _host(std::move(host)), _port(port) {};

    void start();

    void stop();

    void Get(const std::string& path, const HttpHandler& handler);

    void Post(const std::string& path, const HttpHandler& handler);

    void Put(const std::string& path, const HttpHandler& handler);

    void Delete(const std::string& path, const HttpHandler& handler);

    ~HttpServer() = default;

    void setPort(int port) { _port = port; }

    int getPort() const { return _port; }

    std::string getHost() const { return _host; }

    void setHost(std::string host) { _host = std::move(host); }

  private:
    void addRoute(HttpMethod method, const std::string& path, const HttpHandler& handler);

    int _port;
    std::string _host;
  };
} // namespace cppkit::http