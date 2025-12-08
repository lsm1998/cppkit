#include "cppkit/websocket/server.hpp"
#include <iostream>

int main()
{
  using namespace cppkit::websocket;
  using namespace cppkit::http::server;

  WebSocketServer server("127.0.0.1", 8899);

  auto clientMap = std::unordered_map<std::string, std::shared_ptr<ConnInfo>>();

  server.setOnConnect([&](const HttpRequest& request, const ConnInfo& connInfo)
  {
    if (request.getQuery("token") != "yyds")
    {
      connInfo.close();
      return;
    }
    clientMap[connInfo.getClientId()] = std::make_shared<ConnInfo>(connInfo);
    std::cout << "客户端加入:" << connInfo.getClientId() << std::endl;
  });

  server.setOnMessage([&clientMap](const ConnInfo& connInfo, const std::vector<uint8_t>& message, MessageType type)
  {
    // 广播消息给所有客户端
    const std::string msg(message.begin(), message.end());
    std::cout << "Received message from " << connInfo.getClientId() << ": " << msg << std::endl;
    for (const auto& clientId : clientMap | std::views::keys)
    {
      if (connInfo.sendTextMessage("recv:" + msg) < 0)
      {
        perror("sendTextMessage fail");
      }
    }
  });

  server.setOnClose([&](const ConnInfo& connInfo)
  {
    clientMap.erase(connInfo.getClientId());
    std::cout << "客户端离开:" << connInfo.getClientId() << std::endl;
  });

  server.start();
}