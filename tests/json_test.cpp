#include "cppkit/json/json.hpp"
#include <iostream>

using namespace cppkit::json;

int main() {
  auto jsonStr =
      R"({"name":"lsm1998","age":50,"subscribes":["youtobe","qq music",100]})";

  auto json = Json::parse(jsonStr);
  if (json["name"].is_string()) {
    std::cout << json["name"].as_string() << std::endl;
  }
  return 0;
}