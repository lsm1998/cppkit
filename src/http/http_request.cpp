#include "cppkit/http/http_request.hpp"
#include <cstdint>
#include <map>
#include <string>

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
    const std::string& host, const std::string& path, int port, bool https) const
{
  std::ostringstream req;
  req << httpMethodValue(this->method) << " " << path << " HTTP/1.1\r\n";

  if (headers.find("Host") == headers.end())
  {
    if ((https && port != 443) || (!https && port != 80))
      req << "Host: " << host << ":" << port << "\r\n";
    else
      req << "Host: " << host << "\r\n";
  }

  if (headers.find("Connection") == headers.end())
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
    auto pos = line.find(':');
    if (pos != std::string::npos)
      headers[line.substr(0, pos)] = line.substr(pos + 2);
  }

  body.assign(body_part.begin(), body_part.end());
  return HttpResponse(statusCode, headers, body);
}