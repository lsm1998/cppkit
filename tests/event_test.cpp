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
  // define server and event loop
  cppkit::event::EventLoop loop;

  // setup tcp server
  cppkit::event::TcpServer srv(&loop, "127.0.0.1", 6380);

  using ConnInfoPtr = std::shared_ptr<cppkit::event::ConnInfo>;

  // save all active clients
  std::unordered_map<std::string, ConnInfoPtr> clients;

  // new connection established
  srv.setOnConnection(
      [&](const cppkit::event::ConnInfo& info)
      {
        std::cout << "new conn client=" << info.getClientId() << std::endl;
        clients[info.getClientId()] = std::make_shared<cppkit::event::ConnInfo>(info);
      });

  // message received
  srv.setOnMessage(
      [&](const cppkit::event::ConnInfo& info, const std::vector<uint8_t>& msg)
      {
        std::cout << "[msg] " << info.getClientId() << ": " << std::string(msg.begin(), msg.end()) << std::endl;

        // broadcast to all clients
        for (const auto& val : clients | std::views::values)
        {
          if (val->send(msg.data(), msg.size()) < 0)
          {
            perror("send");
          }
        }
      });

  // periodic stats report
  const auto result = loop.createTimeEvent(3000,
      [&](int64_t _)
      {
        std::cout << "[stats] online clients: " << clients.size() << std::endl;
        return 3000; // reschedule after 3 seconds
      });
  if (result < 0)
  {
    std::cerr << "Failed to create time event" << std::endl;
    return -1;
  }

  // client closed connection
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