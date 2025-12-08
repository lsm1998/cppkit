#include "cppkit/websocket/client.hpp"
#include "cppkit/net/socket.hpp"
#include "cppkit/log/log.hpp"
#include "cppkit/crypto/base.hpp"
#include "cppkit/crypto/sha1.hpp"
#include <sstream>
#include <cstring>
#include <netinet/tcp.h>
#include <arpa/inet.h>

#include "cppkit/random.hpp"
#include "cppkit/http/http_response.hpp"

namespace cppkit::websocket
{
  WSClient::~WSClient()
  {
    disconnect();
  }

  bool WSClient::connect(const std::string& url)
  {
    _url = url;
    size_t schemeEnd = url.find("://");
    if (schemeEnd == std::string::npos)
    {
      _onError("Invalid URL format");
      return false;
    }

    if (std::string scheme = url.substr(0, schemeEnd); scheme == "ws")
    {
      _ssl = false;
      _port = 80;
    }
    else if (scheme == "wss")
    {
      _ssl = true;
      _port = 443;
    }
    else
    {
      _onError("Unsupported scheme");
      return false;
    }

    size_t hostStart = schemeEnd + 3;
    if (size_t pathStart = url.find('/', hostStart); pathStart == std::string::npos)
    {
      _host = url.substr(hostStart);
      _path = "/";
    }
    else
    {
      _host = url.substr(hostStart, pathStart - hostStart);
      _path = url.substr(pathStart);
    }

    // Parse port if present
    if (size_t portSep = _host.find(':'); portSep != std::string::npos)
    {
      _port = static_cast<uint16_t>(std::stoi(_host.substr(portSep + 1)));
      _host = _host.substr(0, portSep);
    }

    // Create socket
    _socketFd = socket(AF_INET, SOCK_STREAM, 0);
    if (_socketFd < 0)
    {
      _onError("Failed to create socket");
      return false;
    }

    // Set socket options
    int optVal = 1;
    setsockopt(_socketFd, SOL_SOCKET, SO_REUSEADDR, &optVal, sizeof(optVal));
    setsockopt(_socketFd, IPPROTO_TCP, TCP_NODELAY, &optVal, sizeof(optVal));

    // Resolve host
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(_port);

    if (inet_pton(AF_INET, _host.c_str(), &addr.sin_addr) <= 0)
    {
      // Try to resolve hostname
      hostent* hp = gethostbyname(_host.c_str());
      if (!hp)
      {
        _onError("Failed to resolve host");
        close(_socketFd);
        _socketFd = -1;
        return false;
      }
      std::memcpy(&addr.sin_addr, hp->h_addr_list[0], hp->h_length);
    }

    // Connect to server
    if (::connect(_socketFd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0)
    {
      _onError("Failed to connect to server");
      close(_socketFd);
      _socketFd = -1;
      return false;
    }

    _state = ClientState::CONNECTING;

    // 生成 Sec-WebSocket-Key
    secWebSocketKey = Random::randomString(16, std::string(lowerChars) + upperChars + digitChars);

    // Send WebSocket handshake
    std::stringstream handshake;
    handshake << "GET " << _path << " HTTP/1.1\r\n";
    handshake << "Host: " << _host << ":" << _port << "\r\n";
    handshake << "Upgrade: websocket\r\n";
    handshake << "Connection: Upgrade\r\n";
    handshake << "Sec-WebSocket-Key: " << secWebSocketKey << "\r\n";
    handshake << "Sec-WebSocket-Version: 13\r\n\r\n";

    std::string handshakeStr = handshake.str();
    if (ssize_t sent = ::send(_socketFd, handshakeStr.c_str(), handshakeStr.size(), 0);
      sent != static_cast<ssize_t>(handshakeStr.size()))
    {
      _onError("Failed to send handshake");
      close(_socketFd);
      _socketFd = -1;
      _state = ClientState::DISCONNECTED;
      return false;
    }

    // Read response
    char buffer[1024] = {};
    ssize_t readBytes = recv(_socketFd, buffer, sizeof(buffer) - 1, 0);
    if (readBytes <= 0)
    {
      _onError("Failed to read handshake response");
      close(_socketFd);
      _socketFd = -1;
      _state = ClientState::DISCONNECTED;
      return false;
    }

    std::vector responseData(reinterpret_cast<uint8_t*>(buffer),
        reinterpret_cast<uint8_t*>(buffer + readBytes));
    if (!handleHandshake(responseData))
    {
      _onError("WebSocket handshake failed");
      close(_socketFd);
      _socketFd = -1;
      _state = ClientState::DISCONNECTED;
      return false;
    }

    _state = ClientState::CONNECTED;
    if (_onConnect)
    {
      _onConnect();
    }

    return true;
  }

  void WSClient::disconnect()
  {
    if (_socketFd >= 0)
    {
      // Send close frame
      Frame closeFrame;
      closeFrame.fin = true;
      closeFrame.opCode = MessageType::CLOSE;
      closeFrame.payloadLength = 0;
      const std::vector<uint8_t> frame = buildFrame({}, MessageType::CLOSE);
      ::send(_socketFd, frame.data(), frame.size(), 0);

      close(_socketFd);
      _socketFd = -1;
    }

    _state = ClientState::DISCONNECTED;

    if (_onClose)
    {
      _onClose();
    }
  }

  bool WSClient::send(const std::string& message, const MessageType type) const
  {
    return send(std::vector<uint8_t>(message.begin(), message.end()), type);
  }

  bool WSClient::send(const std::vector<uint8_t>& message, const MessageType type) const
  {
    if (_state != ClientState::CONNECTED || _socketFd < 0)
    {
      return false;
    }

    const std::vector<uint8_t> frame = buildFrame(message, type);
    const ssize_t sent = ::send(_socketFd, frame.data(), frame.size(), 0);

    return sent == static_cast<ssize_t>(frame.size());
  }

  void WSClient::setOnConnect(OnConnectHandler handler)
  {
    _onConnect = std::move(handler);
  }

  void WSClient::setOnMessage(OnMessageHandler handler)
  {
    _onMessage = std::move(handler);
  }

  void WSClient::setOnClose(OnCloseHandler handler)
  {
    _onClose = std::move(handler);
  }

  void WSClient::setOnError(OnErrorHandler handler)
  {
    _onError = std::move(handler);
  }

  bool WSClient::isConnected() const
  {
    return _state == ClientState::CONNECTED;
  }

  bool WSClient::handleHandshake(const std::vector<uint8_t>& data) const
  {
    auto response = http::HttpResponse::parse(data);

    // 是否包含 101 Switching Protocols 状态码
    if (response.getStatusCode() != 101)
    {
      return false;
    }

    // 是否包含 Upgrade: websocket 头
    if (response.getHeader("Upgrade") != "websocket")
    {
      return false;
    }

    // 校验 Sec-WebSocket-Accept 头
    const std::string expectedKey = secWebSocketKey;
    const std::string magicString = expectedKey + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
    const auto sha1Binary = crypto::SHA1::shaBinary(magicString);
    const std::vector sha1Bytes(sha1Binary.begin(), sha1Binary.end());
    if (const std::string accept = crypto::Base64::encode(sha1Bytes); response.getHeader("Sec-WebSocket-Accept") != accept)
    {
      return false;
    }
    return true;
  }
}