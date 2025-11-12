#include "cppkit/crypto/crypto.hpp"
#include <iostream>
#include <ostream>

int main()
{
  const std::string str = "hello world";
  const std::string key = "today";

  // 5eb63bbbe01eeed093cb22bb8f5acdc3
  std::cout << cppkit::crypto::MD5::hash(str) << std::endl;

  // 2aae6c35c94fcfb415dbe95f408b9ce91ee846ed
  cppkit::crypto::SHA1 sha1;
  sha1.update(str);
  std::cout << sha1.hexDigest() << std::endl;

  // b94d27b9934d3e08a52e52d7da7dabfac484efe37a5380ee9088f7ace2efcde9
  cppkit::crypto::SHA256 sha256;
  sha256.update(str);
  std::cout << sha256.hexDigest() << std::endl;

  // 49263232009275ea7b06a79aabe5949acdafcd4f3b0f2300bef802aa5847a7e6
  std::cout << cppkit::crypto::SHA256::hmac(key, str) << std::endl;

  // 309ecc489c12d6eb4cc40f50c902f2b4d0ed77ee511a7c7a9bcd3ca86d4cd86f989dd35bc5ff499670da34255b45b0cfd830e81f605dcf7dc5542e93ae9cd76f
  cppkit::crypto::SHA512 sha512;
  sha512.update(str);
  std::cout << sha512.hexDigest() << std::endl;

  // 随机生成IV
  std::string iv(16, '\0');
  for(auto& c : iv)
    c = static_cast<char>(rand() % 256);
  cppkit::crypto::Example("1234567890abcdef", iv, "这是一段文本-hello");
  return 0;
}