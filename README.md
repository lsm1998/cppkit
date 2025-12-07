# cppkit

A comprehensive C++ utility library with various modules for common programming tasks.

## Features

cppkit provides a collection of modules to simplify C++ development:

- **Crypto**: AES, SHA1, SHA256, SHA512, MD5, base encoding/decoding
- **Networking**: TCP server/client, UDP, socket utilities
- **HTTP**: HTTP server with routing, HTTP client
- **Concurrency**: Thread pool, semaphore, thread group, wait group
- **JSON**: JSON parsing and serialization
- **IO**: File operations
- **Strings**: String utilities
- **Random**: Random number generation
- **Logging**: Logging system
- **Event**: Event loop (ae)
- **Testing**: Unit testing framework
- **Argument Parsing**: Command line argument parsing

## Build

### Prerequisites

- C++20 compiler
- CMake (3.10+)

### Using CMake

```bash
mkdir build && cd build
cmake ..
make
```

## Usage

Here's a simple example of using the TCP server:

```cpp
#include <cppkit/event/server.hpp>
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
```

## Modules

### Crypto

```cpp
#include <cppkit/crypto/crypto.hpp>

// AES encryption/decryption
std::string AESkey = "0123456789abcdef";
std::string iv = "abcdef9876543210";
auto key_bytes = reinterpret_cast<const uint8_t*>(AESkey.data());
const auto* iv_bytes = reinterpret_cast<const uint8_t*>(iv.data());

// Encrypt CBC
std::vector<uint8_t> cipher = cppkit::crypto::AES_Encrypt_CBC(
      std::vector<uint8_t>(plaintext.begin(), plaintext.end()),
      key_bytes,
      iv_bytes);
std::cout << "encryption data (hex): " << cppkit::crypto::toHex(cipher) << "\n";

// Decrypt CBC
std::vector<uint8_t> decrypted = cppkit::crypto::AES_Decrypt_CBC(cipher, key_bytes, iv_bytes);
std::string recovered(decrypted.begin(), decrypted.end());
std::cout << "decryption data: " << recovered << "\n";
```

### HTTP Server

```cpp
#include <cppkit/http/server/http_server.hpp>
#include <iostream>

int main()
{
  using namespace cppkit::http::server;

  // create server
  HttpServer server("127.0.0.1", 8888);

  // register routes

  // GET /hello
  server.Get("/hello",
      [](const HttpRequest& req, HttpResponseWriter& res)
      {
        res.setStatusCode(cppkit::http::HTTP_OK);
        res.setHeader("Content-Type", "text/plain");
        res.write("Hello, World!");
      });

  // GET /hello/:name
  server.Get("/hello/:name",
      [](const HttpRequest& req, HttpResponseWriter& res)
      {
        // extract path parameter
        const std::string name = req.getParam("name");
        res.setStatusCode(cppkit::http::HTTP_OK);
        res.setHeader("Content-Type", "text/plain");
        res.write("Hello, " + name + "!");
      });

  // POST /send
  server.Post("/send",
      [](const HttpRequest& req, HttpResponseWriter& res)
      {
        // read body
        auto body = req.readBody();

        res.setStatusCode(cppkit::http::HTTP_OK);
        res.setHeader("Content-Type", "text/plain");
        res.write("data: " + std::string(body.begin(), body.end()));
      });

  std::cout << "Starting server at " << server.getHost() << ":" << server.getPort() << std::endl;

  // start server
  server.start();
  return 0;
}
```

### Websocket Server

```cpp
#include <cppkit/websocket/server.hpp>
#include <iostream>

int main()
{
  using namespace cppkit::websocket;
  using namespace cppkit::http::server;

  // create websocket server
  WSServer server("127.0.0.1", 8899);

  // map to hold active clients
  auto clientMap = std::unordered_map<std::string, std::shared_ptr<WSConnInfo>>();

  // on new connection
  server.setOnConnect([&](const HttpRequest& request, const WSConnInfo& connInfo)
  {
    // simple token authentication
    if (request.getQuery("token") != "key666")
    {
      connInfo.close();
      return;
    }
    clientMap[connInfo.getClientId()] = std::make_shared<WSConnInfo>(connInfo);
    std::cout << "client join:" << connInfo.getClientId() << std::endl;
  });

  server.setOnMessage([&clientMap](const WSConnInfo& connInfo, const std::vector<uint8_t>& message, MessageType type)
  {
    // broadcast received message to all clients
    const std::string msg(message.begin(), message.end());
    std::cout << "Received message from " << connInfo.getClientId() << ": " << msg << std::endl;
    for (const auto& clientId : clientMap | std::views::keys)
    {
      if (connInfo.sendTextMessage("recv:" + msg) < 0)
      {
        perror("sendTextMessage fail");
      }
    }
  });

  // on client disconnect
  server.setOnClose([&](const WSConnInfo& connInfo)
  {
    clientMap.erase(connInfo.getClientId());
    std::cout << "client exit:" << connInfo.getClientId() << std::endl;
  });

  // start server
  server.start();
}
```

## License

MIT License

