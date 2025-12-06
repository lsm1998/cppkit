#include "cppkit/http/http_request.hpp"
#include <sstream>

namespace cppkit::http
{
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

    for (const auto& [fst, snd] : headers)
    {
      req << fst << ": " << snd << "\r\n";
    }

    if (!body.empty())
    {
      if (!headers.contains("Content-Length"))
        req << "Content-Length: " << body.size() << "\r\n";
    }

    req << "\r\n";

    std::string headerStr = req.str();
    std::vector<uint8_t> data(headerStr.begin(), headerStr.end());
    data.insert(data.end(), body.begin(), body.end());
    return data;
  }
}