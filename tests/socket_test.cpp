#include "cppkit/net/socket.hpp"
#include <iostream>
#include <thread>

void serverDemo()
{
  cppkit::net::Socket socket;
  if (const auto ok = socket.bind("localhost", 9999); !ok)
  {
    perror("bind failed");
    return;
  }
  if (const auto ok = socket.listen(5); !ok)
  {
    std::cerr << "Socket listen error" << std::endl;
    return;
  }
  std::cout << "Server is listening on localhost:9999" << std::endl;
  for (int i = 0; i < 1000; ++i)
  {
    auto client = socket.accept();
    std::cout << "Client accepted,[ip=" << client.getRemoteAddress() << " port=" << client.getRemotePort() << "]" <<
        std::endl;
    char buf[100];
    if (const ssize_t len = client.read(buf, sizeof(buf)); len > 0)
    {
      std::cout << "Received data: " << std::string(buf, len) << std::endl;
      if (client.write(buf, len) <= 0)
      {
        std::cerr << "Socket write error" << std::endl;
      }
    }
  }
}

void clientDemo()
{
  cppkit::net::Socket socket;
  if (const auto ok = socket.connect("localhost", 9999); !ok)
  {
    std::cerr << "Socket connect error" << std::endl;
    return;
  }
  const std::string message = "Hello, Server!";
  if (socket.write(message.data(), message.size()) <= 0)
  {
    std::cerr << "Socket write error" << std::endl;
    return;
  }
  char buf[100];
  if (const size_t len = socket.read(buf, sizeof(buf)); len > 0)
  {
    std::cout << "Received from server: " << std::string(buf, len) << std::endl;
  }
  std::cout << "clientDemo执行完毕" << std::endl;
}

int main()
{
  // 一个线程启动服务器
  std::thread serverThread(serverDemo);
  // 主线程稍作等待，确保服务器先启动
  std::this_thread::sleep_for(std::chrono::seconds(1));

  // 然后启动客户端
  for (std::thread clientThreads[5]; auto& clientThread : clientThreads)
  {
    clientThread = std::thread(clientDemo);
    clientThread.join();
  }

  serverThread.join();
  return 0;
}