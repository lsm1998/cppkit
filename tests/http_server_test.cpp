#include "cppkit/http/server/http_server.hpp"
#include <iostream>

int main()
{
  using namespace cppkit::http::server;

  HttpServer server("127.0.0.1", 8888);

  server.Get("/hello",
      [](const HttpRequest& req, HttpResponseWriter& res)
      {
        res.setStatusCode(cppkit::http::HTTP_OK);
        res.setHeader("Content-Type", "text/plain");
        res.write("Hello, World!");
      });

  server.Get("/hello/:name",
      [](const HttpRequest& req, HttpResponseWriter& res)
      {
        const std::string name = req.getParam("name");
        res.setStatusCode(cppkit::http::HTTP_OK);
        res.setHeader("Content-Type", "text/plain");
        res.write("Hello, " + name + "!");
      });

  server.Post("/send",
      [](const HttpRequest& req, HttpResponseWriter& res)
      {
        auto body = req.readBody();

        res.setStatusCode(cppkit::http::HTTP_OK);
        res.setHeader("Content-Type", "text/plain");
        res.write("data: " + std::string(body.begin(), body.end()));
      });

  std::cout << "Starting server at " << server.getHost() << ":" << server.getPort() << std::endl;
  server.start();
  return 0;
}