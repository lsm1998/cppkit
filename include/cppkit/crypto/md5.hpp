#pragma once

#include "base.hpp"
#include <string>
#include <array>

namespace cppkit::crypto
{
  class MD5 final
  {
  public:
    MD5();

    // 更新哈希计算
    // data: 输入数据  len: 输入数据长度
    void update(const uint8_t* data, size_t len);

    // 更新哈希计算
    // data: 输入数据
    void update(const std::string& data);

    // 获取最终的哈希值
    std::array<uint8_t, 16> digest();

    // 获取十六进制字符串形式的哈希值
    std::string hexDigest();

    // 获取Base64字符串形式的哈希值
    std::string base64Digest();

    // 静态方法，直接计算输入数据的MD5哈希值（十六进制字符串形式）
    static std::string hash(const std::string& data);

    // 静态方法，直接计算输入数据的MD5哈希值（Base64字符串形式）
    static std::string hashBase64(const std::string& data);

  private:
    // MD5变换函数
    // block: 512位输入块
    void transform(const uint8_t block[64]);

    // 完成哈希计算
    void finalize();

    uint64_t bitLen_; // 总位数

    uint32_t state_[4]{}; // A, B, C, D

    uint8_t buffer_[64]{}; // 输入缓冲区

    bool finalized_; // 是否已完成哈希计算
  };
}