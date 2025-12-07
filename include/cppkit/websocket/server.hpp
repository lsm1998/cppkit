#pragma once

#include "frame.hpp"
#include "cppkit/event/server.hpp"
#include "cppkit/http/server/http_request.hpp"
#include <string>
#include <vector>
#include <functional>
#include <unordered_map>
#include <memory>

#include "cppkit/http/http_client.hpp"

namespace cppkit::websocket
{
  // WebSocket 连接信息
  class WSConnInfo
  {
  public:
    explicit WSConnInfo(const event::ConnInfo& connInfo) : _connInfo(connInfo)
    {
    }

    // 获取连接唯一标识
    [[nodiscard]]
    std::string getClientId() const { return _connInfo.getClientId(); }

    // 获取底层连接信息
    [[nodiscard]]
    const event::ConnInfo& getRawConnInfo() const { return _connInfo; }

    // 关闭连接
    void close() const
    {
      _connInfo.close();
    }

  private:
    const event::ConnInfo& _connInfo;
  };

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

    // Send messages
    bool send(const std::string& clientId, const std::string& message, MessageType type = MessageType::TEXT);

    bool send(const std::string& clientId, const std::vector<uint8_t>& message, MessageType type = MessageType::BINARY);

    void start();

    void stop();

    [[nodiscard]] std::string getHost() const;

    [[nodiscard]] int getPort() const;

    ~WSServer();

  private:
    // 处理握手请求
    bool handleHandshake(const event::ConnInfo& connInfo, const std::vector<uint8_t>& data);

    // 解析 WebSocket 帧
    bool parseFrame(const std::vector<uint8_t>& data, Frame& frame);

    // 构建 WebSocket 帧
    std::vector<uint8_t> buildFrame(const std::vector<uint8_t>& payload,
        MessageType type = MessageType::TEXT,
        bool fin = true,
        bool mask = false);

    // TCP 事件处理
    void onTcpConnect(const event::ConnInfo& connInfo);
    void onTcpMessage(const event::ConnInfo& connInfo, const std::vector<uint8_t>& data);
    void onTcpClose(const event::ConnInfo& connInfo);

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