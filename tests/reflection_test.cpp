#include "cppkit/reflection/reflection.hpp"
#include "cppkit/strings.hpp"
#include <vector>
#include <iostream>
#include <map>

struct User
{
    int id;
    std::string name;
    double score;
    std::vector<std::string> roles;
    std::map<std::string, int> tags;
};

REFLECT(User, FIELD(id), FIELD(name), FIELD(score), FIELD(roles), FIELD(tags))

int main()
{
    User user{1001, "Gemini", 99.5, {"admin", "editor"}};
    cppkit::reflection::forEachField(user, [&]<typename T>(const std::string_view name, const T& value)
    {
        using ValueType = std::decay_t<T>;
        std::cout << name << ":";
        if constexpr (std::is_same_v<ValueType, std::string>)
        {
            std::cout << value << std::endl;
        }
        else if constexpr (std::is_arithmetic_v<ValueType>)
        {
            std::cout << std::to_string(value) << std::endl;
        }
        else if constexpr (std::is_same_v<ValueType, std::vector<std::string>>)
        {
            std::cout << "[" << cppkit::join(value, ",") << "]" << std::endl;
        }
    });
}
