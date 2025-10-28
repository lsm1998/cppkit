#include <arpa/inet.h>
#include <chrono>
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <random>
#include <string>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>
#include <vector>

// 单个客户端任务
void clientTask(int id, const std::string &host, int port, int msgCount,
                int intervalMs) {
  int fd = socket(AF_INET, SOCK_STREAM, 0);
  if (fd < 0) {
    perror("socket");
    return;
  }

  sockaddr_in addr{};
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  inet_pton(AF_INET, host.c_str(), &addr.sin_addr);

  if (connect(fd, (sockaddr *)&addr, sizeof(addr)) < 0) {
    perror("connect");
    close(fd);
    return;
  }

  std::cout << "[client " << id << "] connected\n";

  for (int i = 0; i < msgCount; ++i) {
    std::string msg =
        "client#" + std::to_string(id) + " msg#" + std::to_string(i);
    ssize_t n = send(fd, msg.c_str(), msg.size(), 0);
    if (n < 0) {
      perror("send");
      break;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(intervalMs));
  }

  std::cout << "[client " << id << "] done, closing\n";
  close(fd);
}

int main(int argc, char *argv[]) {
  std::string host = "127.0.0.1";
  int port = 6380;

  // 随机数量客户端
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int> clientDist(5, 20);   // 客户端数量
  std::uniform_int_distribution<int> msgDist(3, 10);      // 每个客户端消息数
  std::uniform_int_distribution<int> delayDist(100, 500); // 每条消息间隔

  int clientCount = clientDist(gen);
  std::cout << "[test] launching " << clientCount << " clients...\n";

  std::vector<std::thread> threads;
  threads.reserve(clientCount);

  for (int i = 0; i < clientCount; ++i) {
    int msgCount = msgDist(gen);
    int interval = delayDist(gen);
    threads.emplace_back(clientTask, i + 1, host, port, msgCount, interval);
    std::this_thread::sleep_for(
        std::chrono::milliseconds(50)); // 稍微错开启动时间
  }

  for (auto &t : threads)
    t.join();

  std::cout << "[test] all clients finished\n";
  return 0;
}
