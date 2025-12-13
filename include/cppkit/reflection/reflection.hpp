#pragma once

#include "dynamic.hpp"
#include <string>

namespace cppkit::reflection
{
    namespace internal
    {
        // 确保这里匹配的是 2 个参数的 MethodTag
        template <typename T>
        struct is_method_tag_impl : std::false_type
        {
        };

        template <typename C, typename P>
        struct is_method_tag_impl<MethodTag<C, P>> : std::true_type
        {
        };

        template <typename T>
        struct is_field_tag_impl : std::false_type
        {
        };

        template <typename C, typename... Args>
        struct is_field_tag_impl<FieldTag<C, Args...>> : std::true_type
        {
        };

        template <typename T>
        constexpr bool is_field_tag_v = is_field_tag_impl<T>::value;

        template <typename T>
        constexpr bool is_method_tag_v = is_method_tag_impl<std::decay_t<T>>::value;
    }

    template <typename T, typename Func>
    constexpr void forEachField(T&& object, Func&& func)
    {
        using RawType = std::remove_cvref_t<T>;
        constexpr auto items = MetaData<RawType>::info();

        std::apply([&]<typename... T0>(T0&&... item)
        {
            ((
                [&]
                {
                    using ItemType = std::decay_t<T0>;
                    if constexpr (internal::is_field_tag_v<ItemType>)
                    {
                        func(item.name, object.*item.ptr);
                    }
                }()
            ), ...);
        }, items);
    }

    template <typename T, typename Func>
    constexpr void forEachMethod(T&& object, Func&& func)
    {
        using RawType = std::remove_cvref_t<T>;
        constexpr auto items = MetaData<RawType>::info();

        std::apply([&]<typename... T0>(T0&&... item)
        {
            ((
                [&]
                {
                    using ItemType = std::decay_t<decltype(item)>;
                    if constexpr (internal::is_method_tag_v<ItemType>)
                    {
                        func(item.name, item.ptr);
                    }
                }()
            ), ...);
        }, items);
    }
}
