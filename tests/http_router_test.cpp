#include "cppkit/http/http_router.hpp"
#include <iostream>

void handleTest(const cppkit::http::HttpRequest& req, cppkit::http::HttpResponseWriter& rep)
{
  std::cout << "执行test" << std::endl;
}

int main()
{
  using namespace cppkit::http;

  const Router router{};
  router.addRoute(HttpMethod::Get, "/test/:id", handleTest);
  if (!router.exists(HttpMethod::Get, "/test/123"))
  {
    throw std::runtime_error("路由不存在");
  }
  const auto handler = router.find(HttpMethod::Get, "/test/123");
  const HttpRequest request;
  HttpResponseWriter writer(0);
  handler(request, writer);
  return 0;
}