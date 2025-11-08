#include "cppkit/http/http_server.hpp"
#include <iostream>

int main()
{
  using namespace cppkit::http;

  HttpServer server("localhost", 8080);

  server.Get("/hello",
      [](const HttpRequest& req, HttpResponseWriter& res)
      {
        res.setStatusCode(HTTP_OK);
        res.setHeader("Content-Type", "text/plain");
        res.write("Hello, World!");
      });

  std::cout << "Starting server at " << server.getHost() << ":" << server.getPort() << std::endl;
  server.start();
  return 0;
}