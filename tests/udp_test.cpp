#include "cppkit/net/udp_datagram.hpp"
#include <iostream>
#include <thread>

void udpSendDemo()
{
  const std::string message = "Hello, UDP Server!";
  if (cppkit::net::UdpDatagram udp;
    udp.sendTo("localhost", 8888, message.c_str(), message.size()) <= 0
  )
  {
    perror("UDP sendTo failed");
  }
}

void udpRecvDemo()
{
  cppkit::net::UdpDatagram udp;
  if (!udp.bind("localhost", 8888))
  {
    perror("UDP bind failed");
    return;
  }
  for (int i = 0; i < 1000; ++i)
  {
    char buffer[1024];
    sockaddr_in addr{};
    if (const ssize_t len = udp.recvFrom(buffer, 1024, &addr);
      len > 0
    )
    {
      std::cout << "Received from UDP client: "
          << std::string(buffer, len) << " [ip=" << inet_ntoa(addr.sin_addr)
          << ",port=" << ntohs(addr.sin_port) << "]" << std::endl;
    }
  }
}

int main()
{
  // 一个线程启动UDP接收端
  std::thread serverThread(udpRecvDemo);
  // 主线程稍作等待，确保服务器先启动
  std::this_thread::sleep_for(std::chrono::seconds(1));

  // 发送线程
  for (std::thread clientThreads[5]; auto& clientThread : clientThreads)
  {
    clientThread = std::thread(udpSendDemo);
    clientThread.join();
  }

  serverThread.join();
}