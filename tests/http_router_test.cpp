#include "cppkit/http/http_router.hpp"
#include <iostream>

void handleTest(const std::unordered_map<std::string, std::string>& params)
{
  if (const auto id = params.at("id"); !id.empty())
  {
    std::cout << "id=" << id << std::endl;
  }
  std::cout << "执行test" << std::endl;
}

int main()
{
  using namespace cppkit::http;

  const Router router{};
  router.addRoute(Get, "/test/:id", handleTest);
  if (!router.exists(Get, "/test/123"))
  {
    throw std::runtime_error("路由不存在");
  }
  const auto handler = router.getHandler(Get, "/test/123");
  const std::unordered_map<std::string, std::string> params;
  handler(params);
  return 0;
}