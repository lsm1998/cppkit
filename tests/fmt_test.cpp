#include "cppkit/fmt.hpp"

int main() {
  // 无占位符
  cppkit::print("hello bob");

  // 占位符
  cppkit::print("hello {}", "bob");

  // 多个占位符
  cppkit::print("hello {} {}", 2025, "bob");

  // 转义
  cppkit::print("转义效果{{}");
  return 0;
}