#include "cppkit/websocket/client.hpp"
#include "cppkit/net/socket.hpp"
#include "cppkit/log/log.hpp"
#include "cppkit/crypto/base.hpp"
#include "cppkit/crypto/sha1.hpp"
#include <sstream>
#include <cstring>
#include <netinet/tcp.h>
#include <arpa/inet.h>

namespace cppkit::websocket
{
  WSClient::~WSClient()
  {
    disconnect();
  }

  bool WSClient::connect(const std::string& url)
  {
    // Simple URL parsing
    // Example: ws://localhost:8080/path or wss://localhost:8080/path

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

    // Send WebSocket handshake
    std::stringstream handshake;
    handshake << "GET " << _path << " HTTP/1.1\r\n";
    handshake << "Host: " << _host << ":" << _port << "\r\n";
    handshake << "Upgrade: websocket\r\n";
    handshake << "Connection: Upgrade\r\n";
    handshake << "Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\n";
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
    char buffer[1024] = {0};
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

  bool WSClient::send(const std::string& message, MessageType type)
  {
    return send(std::vector<uint8_t>(message.begin(), message.end()), type);
  }

  bool WSClient::send(const std::vector<uint8_t>& message, MessageType type)
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

  bool WSClient::handleHandshake(const std::vector<uint8_t>& data)
  {
    std::string response(reinterpret_cast<const char*>(data.data()), data.size());

    // Check for successful status code
    if (response.find("HTTP/1.1 101 Switching Protocols") == std::string::npos)
    {
      return false;
    }

    // Check Upgrade header
    if (response.find("Upgrade: websocket") == std::string::npos &&
        response.find("upgrade: websocket") == std::string::npos)
    {
      return false;
    }

    return true;
  }

  bool WSClient::parseFrame(const std::vector<uint8_t>& data, Frame& frame)
  {
    // Same as server's parseFrame method
    if (data.size() < 2)
    {
      return false;
    }

    size_t offset = 0;

    // Parse FIN and Opcode
    frame.fin = (data[offset] & 0x80) != 0;
    frame.opCode = static_cast<MessageType>(data[offset] & 0x0F);
    offset++;

    // Parse Mask and Payload Length
    frame.mask = (data[offset] & 0x80) != 0;
    uint8_t payloadLenByte = data[offset] & 0x7F;
    offset++;

    if (payloadLenByte <= 125)
    {
      frame.payloadLength = payloadLenByte;
    }
    else if (payloadLenByte == 126)
    {
      if (data.size() < offset + 2)
      {
        return false;
      }
      frame.payloadLength = (static_cast<uint16_t>(data[offset]) << 8) | static_cast<uint16_t>(data[offset + 1]);
      offset += 2;
    }
    else
    {
      // 127
      if (data.size() < offset + 8)
      {
        return false;
      }
      frame.payloadLength = 0;
      for (int i = 0; i < 8; ++i)
      {
        frame.payloadLength = (frame.payloadLength << 8) | static_cast<uint64_t>(data[offset + i]);
      }
      offset += 8;
    }

    // Parse Masking Key
    if (frame.mask)
    {
      if (data.size() < offset + 4)
      {
        return false;
      }
      std::memcpy(frame.maskingKey, &data[offset], 4);
      offset += 4;
    }

    // Check if payload is available
    if (data.size() < offset + frame.payloadLength)
    {
      return false;
    }

    // Parse Payload
    frame.payload.resize(frame.payloadLength);
    if (frame.payloadLength > 0)
    {
      std::memcpy(frame.payload.data(), &data[offset], frame.payloadLength);

      // Unmask payload if needed
      if (frame.mask)
      {
        for (uint64_t i = 0; i < frame.payloadLength; ++i)
        {
          frame.payload[i] ^= frame.maskingKey[i % 4];
        }
      }
    }

    return true;
  }

  std::vector<uint8_t> WSClient::buildFrame(const std::vector<uint8_t>& payload, MessageType type, bool fin)
  {
    std::vector<uint8_t> frame;

    // FIN and Opcode
    uint8_t byte = 0;
    if (fin)
    {
      byte |= 0x80;
    }
    byte |= static_cast<uint8_t>(type) & 0x0F;
    frame.push_back(byte);

    // Payload Length (client to server must be masked)
    uint8_t lenByte = 0x80; // Mask is always true for client
    uint64_t payloadLength = payload.size();

    if (payloadLength <= 125)
    {
      lenByte |= static_cast<uint8_t>(payloadLength);
      frame.push_back(lenByte);
    }
    else if (payloadLength <= 65535)
    {
      lenByte |= 126;
      frame.push_back(lenByte);
      frame.push_back(static_cast<uint8_t>((payloadLength >> 8) & 0xFF));
      frame.push_back(static_cast<uint8_t>(payloadLength & 0xFF));
    }
    else
    {
      lenByte |= 127;
      frame.push_back(lenByte);
      // Big-endian
      for (int i = 7; i >= 0; --i)
      {
        frame.push_back(static_cast<uint8_t>((payloadLength >> (i * 8)) & 0xFF));
      }
    }

    // Generate masking key
    uint8_t maskingKey[4] = {
        static_cast<uint8_t>(rand() % 256),
        static_cast<uint8_t>(rand() % 256),
        static_cast<uint8_t>(rand() % 256),
        static_cast<uint8_t>(rand() % 256)
    };
    frame.insert(frame.end(), maskingKey, maskingKey + 4);

    // Payload with masking
    for (uint64_t i = 0; i < payloadLength; ++i)
    {
      frame.push_back(payload[i] ^ maskingKey[i % 4]);
    }

    return frame;
  }
}