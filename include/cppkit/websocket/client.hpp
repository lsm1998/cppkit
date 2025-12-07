#pragma once

#include "frame.hpp"
#include <cppkit/event/server.hpp>
#include <string>
#include <vector>
#include <functional>

namespace cppkit::websocket
{
  // WebSocket client
  class WSClient
  {
  public:
    using OnConnectHandler = std::function<void()>;
    using OnMessageHandler = std::function<void(const std::vector<uint8_t>& MessageType)>;
    using OnCloseHandler = std::function<void()>;
    using OnErrorHandler = std::function<void(const std::string&)>;

    WSClient() = default;

    ~WSClient();

    // Connection
    bool connect(const std::string& url);

    void disconnect();

    // 发送文本消息
    bool send(const std::string& message, MessageType type = MessageType::TEXT);

    // 发送二进制消息
    bool send(const std::vector<uint8_t>& message, MessageType type = MessageType::BINARY);

    // Handlers
    void setOnConnect(OnConnectHandler handler);
    void setOnMessage(OnMessageHandler handler);
    void setOnClose(OnCloseHandler handler);
    void setOnError(OnErrorHandler handler);

    // Status
    bool isConnected() const;

  private:
    // Internal states
    enum class ClientState
    {
      DISCONNECTED,
      CONNECTING,
      CONNECTED
    };

    // WebSocket handshake
    bool handleHandshake(const std::vector<uint8_t>& data);

    // Parse WebSocket frame
    bool parseFrame(const std::vector<uint8_t>& data, Frame& frame);

    // Build WebSocket frame
    std::vector<uint8_t> buildFrame(const std::vector<uint8_t>& payload,
        MessageType type = MessageType::TEXT,
        bool fin = true);

    // Connection info
    std::string _url;
    std::string _host;
    std::string _path;
    uint16_t _port = 80;
    bool _ssl = false;

    ClientState _state = ClientState::DISCONNECTED;
    int _socketFd = -1;

    // Handlers
    OnConnectHandler _onConnect;
    OnMessageHandler _onMessage;
    OnCloseHandler _onClose;
    OnErrorHandler _onError;
  };
}