#include "cppkit/http/http_router.hpp"
#include <iostream>

int main()
{
  using namespace cppkit::http;

  Router router;
  router.addRoute(HttpMethod::Get,
      "/test",
      [](const std::unordered_map<std::string, std::string>& params)
      {
        // Handle GET /test
        std::cout << "Handling GET /test" << std::endl;
      });
  if (router.exists(HttpMethod::Get, "/test"))
  {
    std::cout << "Route /test exists for GET method." << std::endl;
  }
  else
  {
    std::cout << "Route /test does not exist for GET method." << std::endl;
  }
  return 0;
}