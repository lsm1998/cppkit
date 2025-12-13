#pragma once

#include <string>

namespace cppkit::reflection
{
    // 1. 定义一个用于存储 [字段名] 和 [成员指针] 的绑定结构
    template <typename Class, typename T>
    struct Field
    {
        std::string_view name;
        T Class::* ptr; // 成员指针
    };

    template <typename Class, typename T>
    constexpr auto makeField(std::string_view name, T Class::* ptr)
    {
        return Field<Class, T>{name, ptr};
    }

    template <typename T>
    struct MetaData
    {
        static constexpr auto members() { return std::make_tuple(); }
    };

    template <typename T, typename Func>
    constexpr void forEachField(T&& object, Func&& func)
    {
        using RawType = std::remove_cvref_t<T>;

        constexpr auto fields = MetaData<RawType>::members();

        std::apply([&](auto&&... field)
        {
            (func(field.name, object.*field.ptr), ...);
        }, fields);
    }

#define REFLECT(STRUCT_NAME, ...) \
template <> \
struct cppkit::reflection::MetaData<STRUCT_NAME> \
{ \
    using T = STRUCT_NAME; \
    static constexpr auto members() { \
        return std::make_tuple(__VA_ARGS__); \
    } \
};

#define FIELD(MEMBER_NAME) \
cppkit::reflection::makeField(#MEMBER_NAME, &T::MEMBER_NAME)
}
