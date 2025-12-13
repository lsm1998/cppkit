#pragma once

#include "dynamic.hpp"
#include <string>

namespace cppkit::reflection
{
    namespace internal
    {
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

    // Anything 用于占位，可以转换为任意类型的默认值
    struct Anything
    {
        template <typename T>
        constexpr operator T() const
        {
            return T{};
        }
    };

    // 判断类型 T 是否可以用 N 个任意类型的参数进行聚合初始化
    template <typename T, size_t... I>
    consteval bool is_constructible_n(std::index_sequence<I...>)
    {
        return requires { T{(void(I), Anything{})...}; };
    }

    // 递归计算聚合类型 T 的字段数量，N 为当前尝试的字段数量，Max 为最大尝试次数
    template <typename T, size_t N, size_t Max>
    consteval size_t countFieldsImpl()
    {
        static_assert(Max > 0, "Max fields must be greater than 0");

        if constexpr (is_constructible_n<T>(std::make_index_sequence<N>{}))
        {
            if constexpr (N < Max)
            {
                return countFieldsImpl<T, N + 1, Max>();
            }
            else
            {
                return Max;
            }
        }
        else
        {
            return N - 1;
        }
    }

    // 计算聚合类型 T 的字段数量，最多尝试 64 个字段
    template <typename T>
    consteval size_t countFields()
    {
        using RawType = std::decay_t<T>;

        static_assert(std::is_aggregate_v<RawType>, "Type must be an aggregate");

        return countFieldsImpl<RawType, 0, 64>();
    }
}
