#include "cppkit/websocket/client.hpp"
#include <iostream>
#include <ranges>
#include <unistd.h>

int main()
{
  using namespace cppkit::websocket;

  WebSocketClient client;
  if (!client.connect("ws://localhost:8080/ws"))
  {
    perror("");
  }

  client.setOnConnect([]()
  {
    std::cout << "Connected" << std::endl;
  });

  client.setOnClose([]()
  {
    std::cout << "Closed" << std::endl;
  });

  client.setOnError([](const std::string& errMsg)
  {
    std::cout << "Error: " << errMsg << std::endl;
  });

  client.setOnMessage([](const std::vector<uint8_t>& data)
  {
    std::cout << "Received: " << std::string(data.begin(), data.end()) << std::endl;
  });

  for (const auto& _ : std::views::iota(0) | std::views::take(10))
  {
    if (!client.send("hello world"))
    {
      perror("client send failed");
    }
    std::cout << "send once" << std::endl;
    // 休眠200毫秒
    usleep(200 * 1000);
  }
}