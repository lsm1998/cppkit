#include "cppkit/reflection/dynamic.hpp"

namespace cppkit::reflection
{
    Class& Class::forName(const std::string& name)
    {
        auto& [classes] = ClassRegistry::instance();
        if (!classes.contains(name)) throw std::runtime_error("Class not found: " + name);
        return classes[name];
    }
}
