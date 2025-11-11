#include "cppkit/crypto/md5.hpp"

#include <iostream>
#include <ostream>

int main()
{
  const std::string str = "hello world";
  // 5eb63bbbe01eeed093cb22bb8f5acdc3
  std::cout << cppkit::crypto::MD5::hash(str) << std::endl;
}