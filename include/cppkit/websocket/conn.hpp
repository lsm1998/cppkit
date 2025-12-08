#pragma once

#include "frame.hpp"
#include "cppkit/event/server.hpp"

namespace cppkit::websocket
{
  // WebSocket 连接信息
  class ConnInfo
  {
    const event::ConnInfo& _connInfo;

  public:
    explicit ConnInfo(const event::ConnInfo& connInfo) : _connInfo(connInfo)
    {
    }

    // 获取连接唯一标识
    [[nodiscard]]
    std::string getClientId() const;

    // 关闭连接
    void close() const;

    // 获取底层连接信息
    [[nodiscard]]
    const event::ConnInfo& getRawConnInfo() const;

    // 发送文本消息
    [[nodiscard]]
    ssize_t sendTextMessage(const std::string& message) const;

    // 发送二进制消息
    [[nodiscard]]
    ssize_t sendBinaryMessage(const std::vector<uint8_t>& message) const;

    // 发送消息（文本或二进制）
    [[nodiscard]]
    ssize_t sendMessage(const std::vector<uint8_t>& message, MessageType type = MessageType::BINARY) const;
  };
}