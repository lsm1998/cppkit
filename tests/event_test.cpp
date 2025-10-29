#include "cppkit/event/server.hpp"
#include <cstdint>
#include <iostream>
#include <string>
#include <sys/socket.h>
#include <unistd.h>
#include <unordered_map>
#include <utility>
#include <vector>

int main()
{
  cppkit::event::EventLoop loop;
  cppkit::event::TcpServer srv(loop, "", 6380);

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
        for (auto& client : clients)
        {
          if (client.second->send(msg.data(), msg.size()) < 0)
          {
            perror("send");
          }
        }
      });

  // 每隔一秒钟统计当前有多少客户端在线
  loop.createTimeEvent(1000,
                       [&](int64_t id)
                       {
                         std::cout << "[stats] online clients: " << clients.size() << std::endl;
                         return 1000;  // 返回 1000 表示 1 秒后再次触发
                       });

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
