#pragma once

#include "frame.hpp"
#include "conn.hpp"
#include "cppkit/event/server.hpp"
#include "cppkit/http/server/http_request.hpp"
#include "cppkit/http/http_client.hpp"
#include <string>
#include <vector>
#include <functional>
#include <unordered_map>
#include <memory>

namespace cppkit::websocket
{
  // WebSocket 服务器
  class WSServer
  {
  public:
    using Ptr = std::shared_ptr<WSServer>;

    explicit WSServer(const std::string& host = "localhost", uint16_t port = 8080);

    // Handlers
    using OnConnectHandler = std::function<void(const http::server::HttpRequest&, const WSConnInfo&)>;

    using OnMessageHandler = std::function<void(const WSConnInfo&, const std::vector<uint8_t>&, MessageType)>;

    using OnCloseHandler = std::function<void(const WSConnInfo&)>;

    void setOnConnect(OnConnectHandler handler);

    void setOnMessage(OnMessageHandler handler);

    void setOnClose(OnCloseHandler handler);

    bool send(const std::string& clientId, const std::vector<uint8_t>& message, MessageType type = MessageType::BINARY);

    void start();

    void stop();

    [[nodiscard]] std::string getHost() const;

    [[nodiscard]] int getPort() const;

    ~WSServer();

  private:
    // 处理握手请求
    static bool handleHandshake(const event::ConnInfo& connInfo, const std::vector<uint8_t>& data);

    // TCP 事件处理
    void onTcpConnect(const event::ConnInfo& connInfo);
    void onTcpMessage(const event::ConnInfo& connInfo, const std::vector<uint8_t>& data);

    event::EventLoop _loop; // Event loop

    event::TcpServer _tcpServer; // Underlying TCP server

    std::string _host;
    uint16_t _port;

    // Handlers
    OnConnectHandler _onConnect;
    OnMessageHandler _onMessage;
    OnCloseHandler _onClose;

    // Connection states
    std::unordered_map<std::string, ConnData> _connStates;
  };
}