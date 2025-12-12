#include "cppkit/io/file.hpp"
#include <iostream>
#include <stdexcept>

int main()
{
    std::cout << "开始执行" << std::endl;
    const cppkit::io::File file("hello.txt");
    if (!file.exists() && !file.createNewFile())
    {
        throw std::runtime_error("createNewFile执行失败");
    }

    std::cout << file.getAbsolutePath() << std::endl;
    if (!file.deleteOnExit())
    {
        throw std::runtime_error("deleteOnExit执行失败");
    }

    for (const cppkit::io::File dir("build"); auto& item : dir.listFiles())
    {
        std::cout << item.getName() << std::endl;
    }

    std::cout << file.getAbsolutePath() << std::endl;

    constexpr auto str = std::string("hello world");
    for (int i = 0; i < 10; ++i)
    {
        if (const auto n = file.write(str.c_str(), str.size(), 0, true); n <= 0)
        {
            throw std::runtime_error("write执行失败");
        }
    }

    char buffer[1024] = {};
    file.read(buffer, 1024, 0);
    std::cout << std::string(buffer, file.size()) << std::endl;

    const auto total = file.read([](const char* data, const size_t size)
                                 {
                                     std::cout << std::string(data, size) << std::endl;
                                     return true;
                                 },
                                 0,
                                 2);
    std::cout << "读取长度=" << total << std::endl;
}
