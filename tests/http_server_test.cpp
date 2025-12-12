#include "cppkit/http/server/http_server.hpp"
#include "cppkit/json/json.hpp"
#include <iostream>

int main()
{
    using namespace cppkit::http::server;
    using namespace cppkit::json;

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
                    res.setHeader("Content-Type", "application/json");

                    auto result = Json{};
                    result["status"] = 200;

                    if (auto data = Json::parse(std::string(body.begin(), body.end())); data.isObject())
                    {
                        auto obj = data.asObject();
                        result["message"] = obj["name"].asString();
                    }
                    res.write(result.dump());
                });

    // start server
    server.start();
    return 0;
}
