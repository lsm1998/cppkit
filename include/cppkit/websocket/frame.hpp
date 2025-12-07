#pragma once

#include "cppkit/define.hpp"
#include <vector>

namespace cppkit::websocket
{
  // WebSocket 消息类型
  enum class MessageType : uint8_t
  {
    CONTINUATION = 0x0, // 连续帧消息

    TEXT = 0x1, // 文本消息

    BINARY = 0x2, // 二进制消息

    CLOSE = 0x8, // 关闭连接

    PING = 0x9, // 心跳请求

    PONG = 0xA // 心跳响应
  };

  // WebSocket 帧结构
  struct Frame
  {
    bool fin; // FIN 位

    MessageType opCode; // 操作码

    bool mask; // 掩码标志

    uint64_t payloadLength; // 负载长度

    uint8_t maskingKey[4]; // 掩码密钥

    std::vector<uint8_t> payload; // 负载数据
  };

  // 连接状态枚举
  enum class ConnState
  {
    HAND_SHAKING, // 握手中

    CONNECTED // 已连接
  };

  struct ConnData
  {
    ConnState state; // 连接状态

    std::vector<uint8_t> buffer; // 数据缓冲区
  };
} // namespace cppkit::websocket