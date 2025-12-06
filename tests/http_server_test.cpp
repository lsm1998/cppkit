#include "cppkit/http/server/http_server.hpp"
#include <iostream>

int main()
{
  using namespace cppkit::http::server;

  // create server
  HttpServer server("127.0.0.1", 8888);

  // register routes

  // GET /hello
  server.Get("/hello",
      [](const HttpRequest& req, HttpResponseWriter& res)
      {
        res.setStatusCode(cppkit::http::HTTP_OK);
        res.setHeader("Content-Type", "text/plain");
        res.write("Hello, World!");
      });

  // GET /hello/:name
  server.Get("/hello/:name",
      [](const HttpRequest& req, HttpResponseWriter& res)
      {
        // extract path parameter
        const std::string name = req.getParam("name");
        res.setStatusCode(cppkit::http::HTTP_OK);
        res.setHeader("Content-Type", "text/plain");
        res.write("Hello, " + name + "!");
      });

  // POST /send
  server.Post("/send",
      [](const HttpRequest& req, HttpResponseWriter& res)
      {
        // read body
        auto body = req.readBody();

        res.setStatusCode(cppkit::http::HTTP_OK);
        res.setHeader("Content-Type", "text/plain");
        res.write("data: " + std::string(body.begin(), body.end()));
      });

  std::cout << "Starting server at " << server.getHost() << ":" << server.getPort() << std::endl;

  // start server
  server.start();
  return 0;
}