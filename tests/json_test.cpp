#include "cppkit/json/json.hpp"
#include <iostream>

#include "cppkit/json/json_parser.hpp"

using namespace cppkit::json;

struct Address
{
    std::string city;
    int zip{};
};

// 注册 Address
REFLECT(Address, FIELD(city), FIELD(zip))

struct User
{
    int id{};
    std::string name;
    std::vector<double> scores;
    Address address;
    std::vector<Address> old_addresses;
    std::set<std::string> tags;
    std::map<std::string, int> loginLog;
};

// 注册 User
REFLECT(User,
        FIELD(id),
        FIELD(name),
        FIELD(scores),
        FIELD(address),
        FIELD(old_addresses),
        FIELD(tags),
        FIELD(loginLog)
)

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

    User u;
    u.id = 1;
    u.name = "lsm1998";
    u.scores = {99.5, 88.0};
    u.address = {"mast", 10001};
    u.old_addresses = {{"Beijing", 100086}, {"Tokyo", 100000}};
    u.tags = {"Java", "Golang"};
    u.loginLog = {
        {
            "Beijing", 1765609508
        }
    };

    // 万能序列化函数
    std::cout << stringify(u) << std::endl;

    // 万能反序列化函数
    auto t = cppkit::json::fromJson<User>(stringify(u));

    std::cout << t.name << std::endl;
    return 0;
}
