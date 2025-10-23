#include "cppkit/fmt.hpp"

int main() {
  cppkit::print("hello {}", "bob");

  // cppkit::print("转义效果{{}");
  return 0;
}