#pragma once

#include "frame.hpp"
#include "cppkit/event/server.hpp"
#include <string>
#include <vector>
#include <functional>

namespace cppkit::websocket
{
  // WebSocket client
  class WebSocketClient
  {
  public:
    using OnConnectHandler = std::function<void()>;
    using OnMessageHandler = std::function<void(const std::vector<uint8_t>& MessageType)>;
    using OnCloseHandler = std::function<void()>;
    using OnErrorHandler = std::function<void(const std::string&)>;

    WebSocketClient() = default;

    ~WebSocketClient();

    // 连接服务器
    [[nodiscard]]
    bool connect(const std::string& url);

    // 断开连接
    void disconnect();

    // 发送文本消息
    [[nodiscard]]
    bool send(const std::string& message, MessageType type = MessageType::TEXT) const;

    // 发送二进制消息
    [[nodiscard]]
    bool send(const std::vector<uint8_t>& message, MessageType type = MessageType::BINARY) const;

    // Handlers
    void setOnConnect(OnConnectHandler handler);
    void setOnMessage(OnMessageHandler handler);
    void setOnClose(OnCloseHandler handler);
    void setOnError(OnErrorHandler handler);

    // Status
    [[nodiscard]]
    bool isConnected() const;

  private:
    // Internal states
    enum class ClientState
    {
      DISCONNECTED,
      CONNECTING,
      CONNECTED
    };

    // WebSocket 握手处理
    [[nodiscard]]
    bool handleHandshake(const std::vector<uint8_t>& data) const;

    // Connection info
    std::string _url;
    std::string _host;
    std::string _path;
    uint16_t _port = 80;
    bool _ssl = false;

    ClientState _state = ClientState::DISCONNECTED;
    int _socketFd = -1;

    std::string secWebSocketKey;

    // Handlers
    OnConnectHandler _onConnect;
    OnMessageHandler _onMessage;
    OnCloseHandler _onClose;
    OnErrorHandler _onError;
  };
}