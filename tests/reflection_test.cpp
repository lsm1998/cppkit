#include "cppkit/reflection/dynamic.hpp"
#include "cppkit/reflection/reflection.hpp"
#include "cppkit/strings.hpp"
#include <vector>
#include <iostream>
#include <map>

namespace test
{
    struct User
    {
        int id{};
        std::string name;
        double score{};
        std::vector<std::string> roles;
        std::map<std::string, int> tags;

        int getTag(const std::string& key)
        {
            return tags[key];
        }

        [[nodiscard]] std::string sayHello() const
        {
            return "hello: " + name;
        }
    };

    // REFLECT_NAMED(User, "test::User", FIELD(id), FIELD(name), FIELD(score),
    //               FIELD(roles), FIELD(tags), METHOD(getTag), METHOD(sayHello))
}

REFLECT_IN_NS(test, User, FIELD(id), FIELD(name), FIELD(score),
              FIELD(roles), FIELD(tags), METHOD(getTag), METHOD(sayHello))

int main()
{
    test::User user{1001, "lsm1998", 99.5, {"admin", "editor"}};

    cppkit::reflection::forEachField(user, [&]<typename T>(const std::string_view name, const T& value)
    {
        using ValueType = std::decay_t<T>;
        std::cout << name << "->" << cppkit::reflection::getTypeName<ValueType>() << std::endl;
    });

    cppkit::reflection::forEachMethod(user, [&]<typename T>(const std::string_view name, const T& value)
    {
        using ValueType = std::decay_t<T>;
        std::cout << name << "->" << cppkit::reflection::getTypeName<ValueType>() << std::endl;
    });

    auto clazz = cppkit::reflection::Class::forName("test::User");
    std::cout << clazz.name << std::endl;

    auto f = clazz.getField("id");
    f.set(&user, -100);
    std::cout << user.id << std::endl;
}
