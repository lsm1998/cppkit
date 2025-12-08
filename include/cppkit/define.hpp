#pragma once

#include <cstdint>

namespace cppkit
{
  // 默认监听端口
  constexpr int DEFAULT_PORT = 8888;

  // 默认backlog大小
  constexpr int DEFAULT_BACKLOG = 1024;

  // 默认超时时间（秒）
  constexpr int DEFAULT_TIMEOUT = 10;

  // 默认缓冲区大小
  constexpr int DEFAULT_BUFFER_SIZE = 4096;

  namespace http
  {
    class HttpClient;

    class HttpResponse;

    class HttpRequest;

    namespace server
    {
      class HttpServer;
    }
  }

  namespace websocket
  {
    class WsClient;

    class WsServer;
  }
}