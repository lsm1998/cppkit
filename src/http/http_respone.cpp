#include "cppkit/http/http_response.hpp"
#include <sstream>

#include "cppkit/strings.hpp"

namespace cppkit::http
{
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
    const auto val = headers.find(key);
    return val != headers.end() ? val->second : "";
  }

  HttpResponse HttpResponse::parse(const std::vector<uint8_t>& raw)
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
      std::string trimmedLine = trim(line);
      if (trimmedLine.empty())
      {
        break;
      }

      if (auto pos = trimmedLine.find(':'); pos != std::string::npos)
      {
        std::string key = trimmedLine.substr(0, pos);
        std::string value = trimmedLine.substr(pos + 1);
        value = trim(value);

        if (!key.empty())
        {
          headers[key] = value;
        }
      }
    }

    body.assign(body_part.begin(), body_part.end());
    return {statusCode, headers, body};
  }
}