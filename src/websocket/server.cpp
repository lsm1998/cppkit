#include "cppkit/websocket/server.hpp"
#include "cppkit/http/server/http_request.hpp"
#include "cppkit/log/log.hpp"
#include "cppkit/crypto/base.hpp"
#include "cppkit/crypto/sha1.hpp"
#include <algorithm>
#include <sstream>

namespace cppkit::websocket
{
  WSServer::WSServer(const std::string& host, const uint16_t port)
    : _tcpServer(&_loop, host, port), _host(host), _port(port)
  {
  }

  WSServer::~WSServer() = default;

  void WSServer::setOnConnect(OnConnectHandler handler)
  {
    _onConnect = std::move(handler);
  }

  void WSServer::setOnMessage(OnMessageHandler handler)
  {
    _onMessage = std::move(handler);
  }

  void WSServer::setOnClose(OnCloseHandler handler)
  {
    _onClose = std::move(handler);
  }

  bool WSServer::send(const std::string& clientId, const std::string& message, const MessageType type)
  {
    return send(clientId, std::vector<uint8_t>(message.begin(), message.end()), type);
  }

  bool WSServer::send(const std::string& clientId, const std::vector<uint8_t>& message, const MessageType type)
  {
    const std::vector<uint8_t> frame = buildFrame(message, type);

    // We cannot directly use clientId to find in connections_,
    // but WSConnInfo should have it if needed. For now, we assume
    // that clientId is stored in connections_ map - actually, need to fix this!

    // TODO: Implement proper connection ID management

    // For example purposes, iterate through connections
    // Note: This is not efficient, need to fix connection ID mechanism

    const event::ConnInfo* connInfo = nullptr;
    for (const auto& val : event::connections_ | std::views::values)
    {
      if (val.getClientId() == clientId)
      {
        connInfo = &val;
        break;
      }
    }

    if (connInfo == nullptr)
    {
      return false;
    }
    const ssize_t sent = connInfo->send(frame.data(), frame.size());
    return sent == static_cast<ssize_t>(frame.size());
  }

  void WSServer::start()
  {
    // 设置回调函数
    _tcpServer.setOnConnection([this](const event::ConnInfo& connInfo)
    {
      this->onTcpConnect(connInfo);
    });

    _tcpServer.setOnMessage([this](const event::ConnInfo& connInfo, const std::vector<uint8_t>& data)
    {
      this->onTcpMessage(connInfo, data);
    });

    _tcpServer.setOnClose([this](const event::ConnInfo& connInfo)
    {
      if (const std::string clientId = connInfo.getClientId(); _connStates.contains(clientId))
      {
        _connStates.erase(clientId);
      }

      if (_onClose)
      {
        const WSConnInfo wsConnInfo(connInfo);
        _onClose(wsConnInfo);
      }
    });

    _tcpServer.start();
    std::cout << "Started ws server on " << this->_host << ":" << this->_port << std::endl;
    _loop.run();
  }

  void WSServer::stop()
  {
    _loop.stop();
    _tcpServer.stop();
  }

  std::string WSServer::getHost() const
  {
    return _host;
  }

  int WSServer::getPort() const
  {
    return _port;
  }

  void WSServer::onTcpConnect(const event::ConnInfo& connInfo)
  {
    // 初始状态为握手中
    const std::string clientId = connInfo.getClientId();
    _connStates[clientId] = ConnData{ConnState::HAND_SHAKING, {}};
  }

  void WSServer::onTcpMessage(const event::ConnInfo& connInfo, const std::vector<uint8_t>& data)
  {
    const std::string clientId = connInfo.getClientId();
    const auto it = _connStates.find(clientId);

    if (it == _connStates.end())
    {
      return; // 找不到连接状态，忽略消息
    }

    if (auto& [state, buffer] = it->second; state == ConnState::HAND_SHAKING)
    {
      // 完成握手
      if (handleHandshake(connInfo, data))
      {
        state = ConnState::CONNECTED;
        buffer.clear();

        // 通知连接已建立
        if (_onConnect)
        {
          const auto req =
              http::server::HttpRequest::parse(connInfo.getFd(), std::string(data.begin(), data.end()), "");
          _onConnect(req, WSConnInfo(connInfo));
        }
      }
    }
    else if (state == ConnState::CONNECTED) // 已连接状态
    {
      // 追加数据到缓冲区
      buffer.insert(buffer.end(), data.begin(), data.end());

      Frame frame;
      size_t parsedOffset = 0;

      while (true)
      {
        if (buffer.size() < parsedOffset + 2)
        {
          break; // 不完整的帧头
        }

        // 尝试解析帧
        if (parseFrame(std::vector(buffer.begin() + static_cast<long>(parsedOffset), buffer.end()), frame))
        {
          parsedOffset += buffer.size() - (frame.payload.size() + parsedOffset);

          // 处理消息
          if (_onMessage)
          {
            WSConnInfo wsConnInfo(connInfo);
            _onMessage(wsConnInfo, frame.payload, frame.opCode);
          }
        }
        else
        {
          break; // 无法解析完整帧，等待更多数据
        }
      }

      // 移除已解析的数据
      if (parsedOffset > 0)
      {
        if (parsedOffset < buffer.size())
        {
          buffer = std::vector(buffer.begin() + static_cast<long>(parsedOffset), buffer.end());
        }
        else
        {
          buffer.clear();
        }
      }
    }
  }

  bool WSServer::handleHandshake(const event::ConnInfo& connInfo, const std::vector<uint8_t>& data)
  {
    std::string request(reinterpret_cast<const char*>(data.data()), data.size());

    // Find Sec-WebSocket-Key
    std::string key;
    size_t keyPos = request.find("Sec-WebSocket-Key: ");
    if (keyPos == std::string::npos)
    {
      return false;
    }

    size_t keyEnd = request.find("\r\n", keyPos);
    if (keyEnd == std::string::npos)
    {
      return false;
    }

    key = request.substr(keyPos + 19, keyEnd - (keyPos + 19));

    // Generate Sec-WebSocket-Accept
    const std::string magic = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
    std::string combined = key + magic;
    std::string sha1Hash = crypto::SHA1::sha(combined);
    std::vector<uint8_t> sha1Bytes(sha1Hash.begin(), sha1Hash.end());
    std::string accept = crypto::Base64::encode(sha1Bytes);

    // Build response
    std::stringstream response;
    response << "HTTP/1.1 101 Switching Protocols\r\n";
    response << "Upgrade: websocket\r\n";
    response << "Connection: Upgrade\r\n";
    response << "Sec-WebSocket-Accept: " << accept << "\r\n\r\n";

    std::string respStr = response.str();
    ssize_t sent = connInfo.send(reinterpret_cast<const uint8_t*>(respStr.c_str()), respStr.size());

    return sent == static_cast<ssize_t>(respStr.size());
  }

  bool WSServer::parseFrame(const std::vector<uint8_t>& data, Frame& frame)
  {
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
        frame.payloadLength = frame.payloadLength << 8 | static_cast<uint64_t>(data[offset + i]);
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

  std::vector<uint8_t> WSServer::buildFrame(const std::vector<uint8_t>& payload,
      MessageType type,
      bool fin,
      bool mask)
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

    // Mask and Payload Length
    uint8_t lenByte = mask ? 0x80 : 0x00;
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

    // Payload
    frame.insert(frame.end(), payload.begin(), payload.end());

    return frame;
  }

  void WSServer::onTcpClose(const event::ConnInfo& connInfo)
  {
    // connInfo.close();
  }
}