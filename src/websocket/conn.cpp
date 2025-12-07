#include "cppkit/websocket/conn.hpp"
#include "cppkit/websocket/frame.hpp"

namespace cppkit::websocket
{
  std::string WSConnInfo::getClientId() const
  {
    return _connInfo.getClientId();
  }

  void WSConnInfo::close() const
  {
    _connInfo.close();
  }

  const event::ConnInfo& WSConnInfo::getRawConnInfo() const
  {
    return _connInfo;
  }

  ssize_t WSConnInfo::sendTextMessage(const std::string& message) const
  {
    return sendMessage(std::vector<uint8_t>(message.begin(), message.end()), MessageType::TEXT);
  }

  ssize_t WSConnInfo::sendBinaryMessage(const std::vector<uint8_t>& message) const
  {
    return sendMessage(message, MessageType::BINARY);
  }

  ssize_t WSConnInfo::sendMessage(const std::vector<uint8_t>& message, const MessageType type) const
  {
    const auto body = buildFrame(message, type);
    return _connInfo.send(body.data(), body.size());
  }
}