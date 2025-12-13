#pragma once

#include "type.hpp"
#include <iostream>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>
#include <map>
#include <functional>
#include <any>
#include <type_traits>
#include <stdexcept>

namespace cppkit::reflection
{
    // 反射字段结构
    struct ReflectionField
    {
        std::string name;
        std::string typeName;

        std::function<std::any(void*)> getFunc;
        std::function<void(void*, const std::any&)> setFunc;

        std::any get(void* instance) const { return getFunc(instance); }
        void set(void* instance, const std::any& val) const { setFunc(instance, val); }
    };

    // 反射函数结构
    struct ReflectionMethod
    {
        std::string name;
        std::function<std::any(void*, const std::vector<std::any>&)> invoke_func;

        template <typename... Args>
        std::any invoke(void* instance, Args&&... args)
        {
            std::vector<std::any> argList;
            (argList.emplace_back(std::forward<Args>(args)), ...);
            return invoke_func(instance, argList);
        }
    };

    class Class
    {
    public:
        std::string name;
        std::map<std::string, ReflectionField> fields;
        std::map<std::string, ReflectionMethod> methods;

        static Class& forName(const std::string& name);

        ReflectionField& getField(const std::string& fieldName)
        {
            if (!fields.contains(fieldName)) throw std::runtime_error("Field not found: " + fieldName);
            return fields[fieldName];
        }

        ReflectionMethod& getMethod(const std::string& methodName)
        {
            if (!methods.contains(methodName)) throw std::runtime_error("Method not found: " + methodName);
            return methods[methodName];
        }

        std::function<std::any()> constructor;

        [[nodiscard]] std::any newInstance() const
        {
            if (!constructor) throw std::runtime_error("No default constructor registered");
            return constructor();
        }
    };

    // 全局注册表
    class ClassRegistry
    {
    public:
        static ClassRegistry& instance()
        {
            static ClassRegistry r;
            return r;
        }

        std::map<std::string, Class> classes;
    };

    namespace internal
    {
        template <typename T>
        T castAny(const std::any& arg)
        {
            try { return std::any_cast<T>(arg); }
            catch (...) { throw std::runtime_error("Type mismatch in reflection call"); }
        }

        // 字段包装器
        template <typename Class, typename T>
        ReflectionField createDynamicField(const std::string_view name, T Class::* ptr)
        {
            ReflectionField df;
            df.name = name;
            df.typeName = typeid(T).name();

            // 生成 Getter
            df.getFunc = [ptr](void* instance) -> std::any
            {
                return static_cast<Class*>(instance)->*ptr;
            };

            // 生成 Setter
            df.setFunc = [ptr](void* instance, const std::any& val)
            {
                // 注意：这里需要处理类型转换，比如 int -> double 可能会抛出异常
                static_cast<Class*>(instance)->*ptr = castAny<T>(val);
            };
            return df;
        }

        // 方法包装器
        template <typename Class, typename Ret, typename... Args, size_t... I>
        std::any callProxy(Class* obj, Ret (Class::*func)(Args...), const std::vector<std::any>& args,
                           std::index_sequence<I...>)
        {
            if (args.size() != sizeof...(Args)) throw std::runtime_error("Args count mismatch");
            if constexpr (std::is_void_v<Ret>)
            {
                (obj->*func)(castAny<std::decay_t<Args>>(args[I])...);
                return {};
            }
            else
            {
                return (obj->*func)(castAny<std::decay_t<Args>>(args[I])...);
            }
        }

        template <typename Class, typename Ret, typename... Args>
        ReflectionMethod createDynamicMethod(const std::string_view name, Ret (Class::*func)(Args...))
        {
            ReflectionMethod dm;
            dm.name = name;
            dm.invoke_func = [func](void* instance, const std::vector<std::any>& args) -> std::any
            {
                return callProxy(static_cast<Class*>(instance), func, args,
                                 std::make_index_sequence<sizeof...(Args)>{});
            };
            return dm;
        }

        template <typename Class, typename Ret, typename... Args>
        ReflectionMethod createDynamicMethod(std::string_view name, Ret (Class::*func)(Args...) const)
        {
            ReflectionMethod dm;
            dm.name = name;
            dm.invoke_func = [func](void* instance, const std::vector<std::any>& args) -> std::any
            {
                // const 成员函数可以在非 const 指针上调用，所以这里 static_cast<Class*> 依然是安全的
                return call_proxy(static_cast<Class*>(instance), func, args,
                                  std::make_index_sequence<sizeof...(Args)>{});
            };
            return dm;
        }
    }

    template <typename Class, typename T>
    struct FieldTag
    {
        std::string_view name;
        T Class::* ptr;
    };

    template <typename Class, typename Ret, typename... Args>
    struct MethodTag
    {
        std::string_view name;
        Ret (Class::*ptr)(Args...);
    };

    template <typename Class, typename T>
    constexpr auto makeField(std::string_view name, T Class::* ptr)
    {
        return FieldTag<Class, T>{name, ptr};
    }

    template <typename Class, typename Ret, typename... Args>
    constexpr auto makeMethod(std::string_view name, Ret (Class::*ptr)(Args...))
    {
        return MethodTag<Class, Ret, Args...>{name, ptr};
    }

    constexpr auto cppkit_reflect_members(...)
    {
        return std::make_tuple(); // 返回空元组
    }

    template <typename T>
    struct MetaData
    {
        static constexpr auto info()
        {
            return cppkit_reflect_members(static_cast<T*>(nullptr));
        }
    };

    // template <typename T>
    // struct MetaData
    // {
    //     static constexpr auto info() { return std::make_tuple(); } // 默认空
    //     static std::string className() { return ""; }
    // };

    template <typename T>
    struct AutoRegister
    {
        AutoRegister()
        {
            constexpr std::string_view clsNameView = getTypeName<T>();
            const std::string clsName(clsNameView);

            constexpr auto items = MetaData<T>::info();

            Class& cls = ClassRegistry::instance().classes[clsName];
            cls.name = clsName;

            // 注册默认构造函数
            if constexpr (std::is_default_constructible_v<T>)
            {
                cls.constructor = []() -> std::any { return T(); };
            }

            std::apply([&](auto&&... args)
            {
                ((registerItem(cls, args)), ...);
            }, items);
        }

        template <typename Item>
        void registerItem(Class& cls, const Item& item)
        {
            using ItemType = std::decay_t<Item>;

            if constexpr (is_field_tag<ItemType>::value)
            {
                cls.fields[std::string(item.name)] = internal::createDynamicField(item.name, item.ptr);
            }
            else if constexpr (is_method_tag<ItemType>::value)
            {
                cls.methods[std::string(item.name)] = internal::createDynamicMethod(item.name, item.ptr);
            }
        }

        template <typename U>
        struct is_field_tag : std::false_type
        {
        };

        template <typename C, typename V>
        struct is_field_tag<FieldTag<C, V>> : std::true_type
        {
        };

        template <typename U>
        struct is_method_tag : std::false_type
        {
        };

        template <typename C, typename R, typename... A>
        struct is_method_tag<MethodTag<C, R, A...>> : std::true_type
        {
        };
    };

#define REFLECT_VAR_NAME(Line) _auto_reg_##Line

#define REFLECT_IMPL(STRUCT_NAME, LINE, ...) \
inline constexpr auto cppkit_reflect_members(STRUCT_NAME*) { \
using T = STRUCT_NAME; \
return std::make_tuple(__VA_ARGS__); \
} \
inline static cppkit::reflection::AutoRegister<STRUCT_NAME> REFLECT_VAR_NAME(LINE);

#define REFLECT(STRUCT_NAME, ...) \
REFLECT_IMPL(STRUCT_NAME, __LINE__, __VA_ARGS__)

#define REFLECT(STRUCT_NAME, ...) \
REFLECT_IMPL(STRUCT_NAME, __LINE__, __VA_ARGS__)

#define REFLECT_IN_NS(NAMESPACE, STRUCT_NAME, ...) \
namespace NAMESPACE { \
REFLECT(STRUCT_NAME, __VA_ARGS__) \
}

#define FIELD(NAME) \
        cppkit::reflection::makeField(#NAME, &T::NAME)

#define METHOD(NAME) \
        cppkit::reflection::makeMethod(#NAME, &T::NAME)
}
