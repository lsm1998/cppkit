#include "cppkit/http/http_router.hpp"
#include <iostream>

int main()
{
  using namespace cppkit::http;

  const Router router{};
  router.addRoute(HttpMethod::Get,
      "/test",
      [](const std::unordered_map<std::string, std::string>& params)
      {
        std::cout << "执行test" << std::endl;
      });
  if (!router.exists(HttpMethod::Get, "/test"))
  {
    throw std::runtime_error("路由不存在");
  }
  return 0;
}