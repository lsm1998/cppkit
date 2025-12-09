#pragma once

#include "cppkit/define.hpp"
#include <vector>
#include <random>

#include "cppkit/random.hpp"

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

  inline std::vector<uint8_t> buildFrame(const std::vector<uint8_t>& payload,
      MessageType type,
      const bool fin = true,
      const bool mask = false)
  {
    std::vector<uint8_t> frame;
    const uint64_t payloadLength = payload.size();

    std::vector<uint8_t> maskedPayload = payload;

    uint8_t byte0 = 0;
    if (fin)
    {
      byte0 |= 0x80; // 设置 FIN 位
    }
    byte0 |= static_cast<uint8_t>(type) & 0x0F; // 设置 Opcode
    frame.push_back(byte0);

    uint8_t lenByte1 = mask ? 0x80 : 0x00; // 设置 MASK 位

    if (payloadLength <= 125)
    {
      lenByte1 |= static_cast<uint8_t>(payloadLength);
      frame.push_back(lenByte1);
    }
    else if (payloadLength <= 65535)
    {
      lenByte1 |= 126;
      frame.push_back(lenByte1);

      frame.push_back(static_cast<uint8_t>(payloadLength >> 8 & 0xFF));
      frame.push_back(static_cast<uint8_t>(payloadLength & 0xFF));
    }
    else
    {
      lenByte1 |= 127;
      frame.push_back(lenByte1);

      for (int i = 7; i >= 0; --i)
      {
        frame.push_back(static_cast<uint8_t>(payloadLength >> (i * 8) & 0xFF));
      }
    }

    // 处理掩码
    if (mask)
    {
      std::vector<uint8_t> maskingKey(4);

      for (size_t i = 0; i < 4; ++i)
      {
        maskingKey[i] = static_cast<uint8_t>(Random::nextInt(0, 255));
      }

      frame.insert(frame.end(), maskingKey.begin(), maskingKey.end());

      for (size_t i = 0; i < maskedPayload.size(); ++i)
      {
        maskedPayload[i] ^= maskingKey[i % 4];
      }
    }

    frame.insert(frame.end(), maskedPayload.begin(), maskedPayload.end());
    return frame;
  }

  inline size_t parseFrame(const std::vector<uint8_t>& data, Frame& frame)
  {
    if (data.size() < 2)
    {
      return 0;
    }

    size_t offset = 0;

    // Parse FIN and Opcode
    frame.fin = (data[offset] & 0x80) != 0;
    frame.opCode = static_cast<MessageType>(data[offset] & 0x0F);
    offset++;

    // Parse Mask and Payload Length
    frame.mask = (data[offset] & 0x80) != 0;
    uint8_t payloadLenByte = data[offset] & 0x7F;
    offset++;

    if (payloadLenByte <= 125)
    {
      frame.payloadLength = payloadLenByte;
    }
    else if (payloadLenByte == 126)
    {
      if (data.size() < offset + 2)
      {
        return 0;
      }
      frame.payloadLength = (static_cast<uint16_t>(data[offset]) << 8) | static_cast<uint16_t>(data[offset + 1]);
      offset += 2;
    }
    else
    {
      // 127
      if (data.size() < offset + 8)
      {
        return 0;
      }
      frame.payloadLength = 0;
      for (int i = 0; i < 8; ++i)
      {
        frame.payloadLength = frame.payloadLength << 8 | static_cast<uint64_t>(data[offset + i]);
      }
      offset += 8;
    }

    // Parse Masking Key
    if (frame.mask)
    {
      if (data.size() < offset + 4)
      {
        return 0;
      }
      std::memcpy(frame.maskingKey, &data[offset], 4);
      offset += 4;
    }

    // Check if payload is available
    if (data.size() < offset + frame.payloadLength)
    {
      return 0;
    }

    // Parse Payload
    frame.payload.resize(frame.payloadLength);
    if (frame.payloadLength > 0)
    {
      std::memcpy(frame.payload.data(), &data[offset], frame.payloadLength);

      // Unmask payload if needed
      if (frame.mask)
      {
        for (uint64_t i = 0; i < frame.payloadLength; ++i)
        {
          frame.payload[i] ^= frame.maskingKey[i % 4];
        }
      }
    }

    // 返回解析的总字节数：offset + payloadLength
    return offset + frame.payloadLength;
  }
} // namespace cppkit::websocket