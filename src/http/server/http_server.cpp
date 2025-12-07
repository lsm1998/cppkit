#include "cppkit/http/server/http_server.hpp"
#include "cppkit/event/server.hpp"
#include <iostream>

namespace cppkit::http::server
{
  void HttpServer::start()
  {
    this->_server = event::TcpServer(&this->_loop, this->_host, static_cast<uint16_t>(this->_port));

    this->_server.setReadable([this](const event::ConnInfo& conn)
    {
      HttpResponseWriter writer(conn.getFd());

      // 解析HTTP请求
      const HttpRequest request = HttpRequest::parse(conn.getFd());

      // 处理请求
      handleRequest(request, writer);
      return 0;
    });
    this->_server.start();
    this->_loop.run();
  }

  void HttpServer::stop()
  {
    this->_loop.stop();
    this->_server.stop();
  }

  void HttpServer::Get(const std::string& path, const HttpHandler& handler) const
  {
    this->addRoute(HttpMethod::Get, path, handler);
  }

  void HttpServer::Post(const std::string& path, const HttpHandler& handler) const
  {
    this->addRoute(HttpMethod::Post, path, handler);
  }

  void HttpServer::Put(const std::string& path, const HttpHandler& handler) const
  {
    this->addRoute(HttpMethod::Put, path, handler);
  }

  void HttpServer::Delete(const std::string& path, const HttpHandler& handler) const
  {
    this->addRoute(HttpMethod::Delete, path, handler);
  }

  void HttpServer::addRoute(const HttpMethod method, const std::string& path, const HttpHandler& handler) const
  {
    // 判断路由是否已存在
    if (this->_router.exists(method, path))
    {
      throw std::runtime_error("Route already exists: " + httpMethodValue(method) + " " + path);
    }
    _router.addRoute(method, path, handler);
    std::cout << "Added route: " << httpMethodValue(method) << " " << path << std::endl;
  }

  void HttpServer::handleRequest(const HttpRequest& request, HttpResponseWriter& writer) const
  {
    std::unordered_map<std::string, std::string> params;
    const auto handler = _router.find(request.getMethod(), request.getPath(), params);
    if (handler == nullptr)
    {
      writer.setStatusCode(HTTP_NOT_FOUND);
      writer.setHeader("Content-Type", "text/plain");
      writer.write("404 Not Found");
      return;
    }
    request.setParams(params);
    handler(request, writer);
  }
} // namespace cppkit::http