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

  std::cout << "Starting server at " << server.getHost() << ":" << server.getPort() << std::endl;
  server.start();
  return 0;
}