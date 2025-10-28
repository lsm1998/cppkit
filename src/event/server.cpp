#include "cppkit/event/server.hpp"
#include <arpa/inet.h>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>

namespace cppkit::event {

static int setNonBlock(int fd) {
  int flags = fcntl(fd, F_GETFL, 0);
  if (flags == -1)
    return -1;
  if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1)
    return -1;
  return 0;
}

TcpServer::TcpServer(EventLoop &loop, const std::string &addr, uint16_t port)
    : loop_(loop), addr_(addr), port_(port) {}

TcpServer::~TcpServer() {
  if (listenfd_ != -1)
    close(listenfd_);
}

void TcpServer::start() {
  listenfd_ = socket(AF_INET, SOCK_STREAM, 0);
  if (listenfd_ < 0)
    throw std::runtime_error(std::string("socket: ") + strerror(errno));
  int on = 1;
  setsockopt(listenfd_, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
  sockaddr_in sa{};
  sa.sin_family = AF_INET;
  sa.sin_port = htons(port_);
  sa.sin_addr.s_addr = addr_.empty() ? INADDR_ANY : inet_addr(addr_.c_str());
  if (bind(listenfd_, (sockaddr *)&sa, sizeof(sa)) < 0)
    throw std::runtime_error(std::string("bind: ") + strerror(errno));
  if (listen(listenfd_, SOMAXCONN) < 0)
    throw std::runtime_error(std::string("listen: ") + strerror(errno));
  setNonBlock(listenfd_);
  // register accept handler
  loop_.createFileEvent(listenfd_, AE_READABLE, [this](int fd, int mask) {
    while (true) {
      sockaddr_in cli{};
      socklen_t cli_len = sizeof(cli);
      int c = accept(fd, (sockaddr *)&cli, &cli_len);
      if (c < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
          break;
        std::cerr << "accept error: " << strerror(errno) << "\n";
        break;
      }
      setNonBlock(c);

      char ipBuf[64];
      inet_ntop(AF_INET, &cli.sin_addr, ipBuf, sizeof(ipBuf));
      uint16_t port = ntohs(cli.sin_port);
      connections_[c] = ConnInfo{ipBuf, port, c};

      if (onConn_) {
        onConn_(c);
      }
      // create read handler for client
      loop_.createFileEvent(c, AE_READABLE, [this](int cfd, int cmask) {
        // read data
        char buf[4096];
        ssize_t n = read(cfd, buf, sizeof(buf));
        if (n > 0) {
          if (onMsg_)
            onMsg_(connections_[cfd], std::vector<uint8_t>(buf, buf + n));
        } else {
          // closed or error
          loop_.deleteFileEvent(cfd, AE_READABLE | AE_WRITABLE);
          close(cfd);
        }
      });

      // create close handler for client
      loop_.createFileEvent(c, AE_READABLE, [this](int cfd, int cmask) {
        char buf[4096];
        ssize_t n = read(cfd, buf, sizeof(buf));
        if (n > 0) {
          if (onMsg_) {
            onMsg_(connections_[cfd], std::vector<uint8_t>(buf, buf + n));
          }
        } else {
          if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            // 非阻塞下无数据，忽略
            return;
          }
          // 清理事件并关闭连接
          loop_.deleteFileEvent(cfd, AE_READABLE | AE_WRITABLE);
          close(cfd);
          if (onClose_) {
            onClose_(cfd);
          }
          connections_.erase(cfd);
        }
      });
    }
  });
}

void TcpServer::stop() {
  loop_.deleteFileEvent(listenfd_, AE_READABLE);
  if (listenfd_ != -1) {
    close(listenfd_);
  }
  listenfd_ = -1;
}

} // namespace cppkit::event