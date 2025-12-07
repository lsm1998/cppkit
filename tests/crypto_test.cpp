#include "cppkit/crypto/crypto.hpp"
#include "cppkit/random.hpp"
#include <iostream>

int main()
{
  const std::string str = "hello world";
  const std::string key = "today";

  assert(cppkit::crypto::MD5::hash(str) == "5eb63bbbe01eeed093cb22bb8f5acdc3");

  cppkit::crypto::SHA1 sha1;
  sha1.update(str);
  assert(sha1.hexDigest()=="2aae6c35c94fcfb415dbe95f408b9ce91ee846ed");

  cppkit::crypto::SHA256 sha256;
  sha256.update(str);
  assert(sha256.hexDigest()=="b94d27b9934d3e08a52e52d7da7dabfac484efe37a5380ee9088f7ace2efcde9");

  assert(cppkit::crypto::SHA256::hmac(key, str)=="49263232009275ea7b06a79aabe5949acdafcd4f3b0f2300bef802aa5847a7e6");

  cppkit::crypto::SHA512 sha512;
  sha512.update(str);
  assert(
      sha512.hexDigest()==
      "309ecc489c12d6eb4cc40f50c902f2b4d0ed77ee511a7c7a9bcd3ca86d4cd86f989dd35bc5ff499670da34255b45b0cfd830e81f605dcf7dc5542e93ae9cd76f");

  // 随机生成IV
  std::string iv(16, '\0');
  for (auto& c : iv)
    c = static_cast<char>(cppkit::Random::nextInt(1000) % 256);
  const std::string AESkey = "1234567890abcdef";
  const std::string plaintext = "这是一段文本-hello";

  auto key_bytes = reinterpret_cast<const uint8_t*>(AESkey.data());
  const auto* iv_bytes = reinterpret_cast<const uint8_t*>(iv.data());

  // Encrypt CBC
  std::vector<uint8_t> cipher = cppkit::crypto::AES_Encrypt_CBC(
      std::vector<uint8_t>(plaintext.begin(), plaintext.end()),
      key_bytes,
      iv_bytes);
  std::cout << "加密后数据 (hex): " << cppkit::crypto::toHex(cipher) << "\n";

  // Decrypt CBC
  std::vector<uint8_t> decrypted = cppkit::crypto::AES_Decrypt_CBC(cipher, key_bytes, iv_bytes);
  std::string recovered(decrypted.begin(), decrypted.end());
  std::cout << "加密前数据: " << recovered << "\n";
  return 0;
}