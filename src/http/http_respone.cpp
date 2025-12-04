#include "cppkit/http/http_respone.hpp"
#include <sys/socket.h>
#include <sstream>
#include <unordered_map>

// 定义HTTP状态码映射
const std::unordered_map<int, std::string> HTTP_STATUS_MAP = {
    {200, "OK"},
    {404, "Not Found"},
    {500, "Internal Server Error"}
    // 可以根据需要添加更多
};

using namespace cppkit::http;

int HttpResponse::getStatusCode() const
{
  return this->statusCode;
}

std::vector<uint8_t> HttpResponse::getBody() const
{
  return this->body;
}

std::map<std::string, std::string> HttpResponse::getHeaders() const
{
  return this->headers;
}

std::string HttpResponse::getHeader(const std::string& key) const
{
  return this->headers.at(key);
}

std::vector<uint8_t> HttpRequest::buildRequestData(
    const std::string& host,
    const std::string& path,
    const int port,
    const bool https) const
{
  std::ostringstream req;
  req << httpMethodValue(this->method) << " " << path << " HTTP/1.1\r\n";

  if (!headers.contains("Host"))
  {
    if ((https && port != 443) || (!https && port != 80))
      req << "Host: " << host << ":" << port << "\r\n";
    else
      req << "Host: " << host << "\r\n";
  }

  if (!headers.contains("Connection"))
  {
    req << "Connection: close\r\n";
  }

  for (const auto& h : headers)
  {
    req << h.first << ": " << h.second << "\r\n";
  }

  if (!body.empty())
  {
    if (headers.find("Content-Length") == headers.end())
      req << "Content-Length: " << body.size() << "\r\n";
  }

  req << "\r\n";

  std::string headerStr = req.str();
  std::vector<uint8_t> data(headerStr.begin(), headerStr.end());
  data.insert(data.end(), body.begin(), body.end());
  return data;
}

HttpResponse HttpResponse::parseResponse(const std::vector<uint8_t>& raw)
{
  std::string text(raw.begin(), raw.end());
  size_t header_end = text.find("\r\n\r\n");
  if (header_end == std::string::npos)
  {
    return {};
  }

  std::string header_part = text.substr(0, header_end);
  std::string body_part = text.substr(header_end + 4);

  std::istringstream stream(header_part);
  std::string line;
  std::getline(stream, line);
  int statusCode{};
  std::map<std::string, std::string> headers{};
  std::vector<uint8_t> body{};
  if (line.starts_with("HTTP/"))
  {
    statusCode = std::stoi(line.substr(9, 3));
  }

  while (std::getline(stream, line) && !line.empty())
  {
    if (auto pos = line.find(':'); pos != std::string::npos)
      headers[line.substr(0, pos)] = line.substr(pos + 2);
  }

  body.assign(body_part.begin(), body_part.end());
  return {statusCode, headers, body};
}

void HttpResponseWriter::setStatusCode(const int statusCode)
{
  this->statusCode = statusCode;
}

void HttpResponseWriter::setHeaders(const std::map<std::string, std::string>& headers)
{
  this->headers = headers;
}

void HttpResponseWriter::setHeader(const std::string& key, const std::string& value)
{
  this->headers[key] = value;
}

ssize_t HttpResponseWriter::write(const std::string& body)
{
  return write(std::vector<uint8_t>(body.begin(), body.end()));
}

ssize_t HttpResponseWriter::write(const std::vector<uint8_t>& body)
{
  static const size_t BUFFER_SIZE = 4096; // 定义缓冲区大小
  ssize_t totalBytesSent = 0;
  std::ostringstream response;

  // 状态行：添加状态码描述
  auto status_it = HTTP_STATUS_MAP.find(this->statusCode);
  std::string status_desc = status_it != HTTP_STATUS_MAP.end() ? status_it->second : "Unknown";
  response << "HTTP/1.1 " << this->statusCode << " " << status_desc << "\r\n";

  // 响应头
  for (const auto& [key, value] : this->headers)
  {
    response << key << ": " << value << "\r\n";
  }
  response << "Content-Length: " << body.size() << "\r\n";
  response << "Connection: close\r\n"; // 关闭连接
  response << "\r\n";

  std::string header_str = response.str();
  totalBytesSent += ::send(fd, header_str.data(), header_str.size(), 0);

  // 分批次发送响应体
  size_t bytesSent = 0;
  const size_t bodySize = body.size();
  while (bytesSent < bodySize)
  {
    const size_t chunkSize = std::min(BUFFER_SIZE, bodySize - bytesSent);
    const ssize_t n = ::send(fd, &body[bytesSent], chunkSize, 0);
    if (n <= 0)
    {
      break;
    }
    bytesSent += n;
    totalBytesSent += n;
  }
  return totalBytesSent;
}