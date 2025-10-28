#pragma once

#include "ae.hpp"
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace cppkit::event {

struct ConnInfo {
  std::string ip;
  uint16_t port;
  int fd;
};

std::unordered_map<int, ConnInfo> connections_;

class TcpServer {
public:
  using OnConnection = std::function<void(int fd)>;
  using OnMessage =
      std::function<void(const ConnInfo &conn, const std::vector<uint8_t> &)>;
  using OnClose = std::function<void(int fd)>;

  TcpServer(EventLoop &loop, const std::string &addr, uint16_t port);
  ~TcpServer();
  void start();
  void stop();

  void setOnConnection(OnConnection cb) { onConn_ = std::move(cb); }
  void setOnMessage(OnMessage cb) { onMsg_ = std::move(cb); }
  void setOnClose(OnClose cb) { onClose_ = std::move(cb); }

private:
  EventLoop &loop_;
  int listenfd_ = -1;
  std::string addr_;
  uint16_t port_;
  OnConnection onConn_;
  OnMessage onMsg_;
  OnClose onClose_;

  // static void acceptHandler(EventLoop *loop, int fd, int mask,
  //                           TcpServer *server);
};

} // namespace cppkit::event