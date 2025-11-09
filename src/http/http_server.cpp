#include "cppkit/http/http_server.hpp"

#include <iostream>

#include "cppkit/event/server.hpp"

namespace cppkit::http
{
  void HttpServer::start()
  {
    this->_server = event::TcpServer(&this->_loop, this->_host, static_cast<uint16_t>(this->_port));
    this->_server.setOnMessage([this](const event::ConnInfo& conn, const std::vector<uint8_t>& data)
    {
      HttpResponseWriter writer(conn.getFd());
      const HttpRequest request;
      handleRequest(request, writer);
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
    _router.addRoute(method, path, handler);
  }

  void HttpServer::handleRequest(const HttpRequest& request, HttpResponseWriter& writer) const
  {
    const auto handler = _router.find(request.method, request.getPath());
    if (handler == nullptr)
    {
      writer.setStatusCode(HTTP_NOT_FOUND);
      writer.setHeader("Content-Type", "text/plain");
      writer.write("404 Not Found");
      return;
    }
    handler(request, writer);
  }
} // namespace cppkit::http