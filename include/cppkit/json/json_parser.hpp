#pragma once

#include "json.hpp"

namespace cppkit::json
{
    class ReflectionParser
    {
    public:
        template <typename T>
        static void fromJson(const Json& obj, T& out)
        {
            if (obj.isNull())
            {
                return;
            }
            using Type = std::decay_t<T>;

            if constexpr (std::is_same_v<Type, Json>)
            {
                out = obj;
            }

            if constexpr (std::is_arithmetic_v<Type>) //算术类型
            {
                if constexpr (std::is_same_v<Type, bool>)
                {
                    if (obj.isBool()) out = obj.asBool();
                    else throw std::runtime_error("Type mismatch: expected bool");
                }
                else
                {
                    if (obj.isNumber()) out = static_cast<Type>(obj.asNumber());
                    else throw std::runtime_error("Type mismatch: expected number");
                }
            }
            else if constexpr (std::is_convertible_v<Type, std::string>) // 字符串
            {
                if (obj.isString()) out = obj.asString();
                else throw std::runtime_error("Type mismatch: expected string");
            }
            else if constexpr (internal::is_sequence_container_v<Type>) // 序列容器
            {
                if (!obj.isArray()) throw std::runtime_error("Type mismatch: expected array");

                const auto& arr = obj.asArray();
                out.clear();
                if constexpr (std::is_same_v<Type, std::vector<typename Type::value_type>>)
                {
                    out.reserve(arr.size());
                }

                for (const auto& item : arr)
                {
                    typename Type::value_type val{};
                    fromJson(item, val); // 递归
                    out.push_back(std::move(val));
                }
            }
            else if constexpr (internal::is_set_container_v<Type>) // set容器
            {
                if (!obj.isArray()) throw std::runtime_error("Type mismatch: expected array for set");
                const auto& arr = obj.asArray();
                out.clear();
                for (const auto& item : arr)
                {
                    typename Type::value_type val{};
                    fromJson(item, val);
                    out.insert(std::move(val));
                }
            }
            else if constexpr (internal::is_map_container_v<Type>) // map容器
            {
                if (!obj.isObject()) throw std::runtime_error("Type mismatch: expected object for map");

                const auto& item = obj.asObject();
                out.clear();
                for (const auto& [jsonKey, jsonVal] : item)
                {
                    typename Type::mapped_type mapVal{};
                    fromJson(jsonVal, mapVal); // 递归转换 Value

                    // 处理 Key (JSON Key 永远是 string)
                    if constexpr (std::is_arithmetic_v<typename Type::key_type>)
                    {
                        // 如果 Map 的 Key 是数字 (map<int, ...>)
                        if constexpr (std::is_floating_point_v<typename Type::key_type>)
                            out.emplace(std::stod(jsonKey), std::move(mapVal));
                        else
                            out.emplace(std::stoll(jsonKey), std::move(mapVal));
                    }
                    else
                    {
                        out.emplace(jsonKey, std::move(mapVal));
                    }
                }
            }
            else if constexpr (internal::is_reflectable_v<Type>) // 自定义类型
            {
                if (!obj.isObject()) throw std::runtime_error("Type mismatch: expected object for struct");

                const auto& objMap = obj.asObject();

                // 利用反射遍历 C++ 结构体的字段
                reflection::forEachField(out, [&](std::string_view name, auto& field)
                {
                    // 在 JSON Map 中查找该字段名
                    const std::string key(name);
                    if (const auto it = objMap.find(key); it != objMap.end())
                    {
                        // 找到后，递归转换
                        fromJson(it->second, field);
                    }
                });
            }
            else
            {
                static_assert(std::is_void_v<Type>, "Unsupported type for Json conversion");
            }
        }

        template <typename T>
        static void fromJson(const std::string_view str, T& out)
        {
            auto obj = Json::parse(std::string(str));
            fromJson(obj, out);
        }
    };

    template <typename T>
    T fromJson(const std::string_view json)
    {
        T obj{};
        ReflectionParser::fromJson<T>(json, obj);
        return obj;
    }
}
