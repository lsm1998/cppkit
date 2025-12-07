#include "cppkit/json/json.hpp"
#include <iostream>

using namespace cppkit::json;

int main(int argc, char* argv[])
{
  const auto jsonStr = R"({"name":"lsm1998","age":50,"subscribes":["YouTube","qq music",100]})";

  if (auto json = Json::parse(jsonStr); json["name"].isString())
  {
    std::cout << json["name"].asString() << std::endl;
  }

  auto obj = Json{};
  obj["greeting"] = "Hello, JSON!";
  obj["year"] = 2025;

  obj["name"] = Json("bob");
  obj["is_active"] = Json(true);

  std::cout << obj.dump() << std::endl;
  return 0;
}