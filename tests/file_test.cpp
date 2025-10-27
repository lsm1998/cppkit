#include "cppkit/io/file.hpp"
#include <iostream>
#include <stdexcept>

int main() {
  std::cout << "开始执行" << std::endl;
  cppkit::io::File file("hello.txt");
  if (!file.exists() && !file.createNewFile()) {
    throw std::runtime_error("createNewFile执行失败");
  }

  std::cout << file.getAbsolutePath() << std::endl;
  file.deleteOnExit();

  cppkit::io::File dir("build");
  for (auto &item : dir.listFiles()) {
    std::cout << item.getName() << std::endl;
  }
  return 0;
}