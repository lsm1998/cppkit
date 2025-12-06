#include "cppkit/event/server.hpp"
#include <iostream>
#include <ranges>
#include <string>
#include <sys/socket.h>
#include <unordered_map>
#include <vector>
#include <memory>
#include <ranges>

int main()
{
  cppkit::event::EventLoop loop;
  cppkit::event::TcpServer srv(&loop, "127.0.0.1", 6380);

  using ConnInfoPtr = std::shared_ptr<cppkit::event::ConnInfo>;

  // 保存所有活跃连接
  std::unordered_map<std::string, ConnInfoPtr> clients;

  // 新连接
  srv.setOnConnection(
      [&](const cppkit::event::ConnInfo& info)
      {
        std::cout << "new conn client=" << info.getClientId() << std::endl;
        clients[info.getClientId()] = std::make_shared<cppkit::event::ConnInfo>(info);
      });

  // 接收到消息
  srv.setOnMessage(
      [&](const cppkit::event::ConnInfo& info, const std::vector<uint8_t>& msg)
      {
        std::cout << "[msg] " << info.getClientId() << ": " << std::string(msg.begin(), msg.end()) << std::endl;

        // 广播给所有活跃客户端
        for (const auto& val : clients | std::views::values)
        {
          if (val->send(msg.data(), msg.size()) < 0)
          {
            perror("send");
          }
        }
      });

  // 每隔3秒钟统计当前有多少客户端在线
  const auto result = loop.createTimeEvent(3000,
      [&](int64_t _)
      {
        std::cout << "[stats] online clients: " << clients.size() << std::endl;
        return 3000; // 返回 3000 表示 3 秒后再次触发
      });
  if (result < 0)
  {
    std::cerr << "Failed to create time event" << std::endl;
    return -1;
  }

  // 客户端关闭
  srv.setOnClose(
      [&](const cppkit::event::ConnInfo& info)
      {
        std::cout << "client closed client=" << info.getClientId() << "\n";
        clients.erase(info.getClientId());
      });

  srv.start();
  std::cout << "Broadcast server running on port 6380" << std::endl;
  loop.run();
  return 0;
}