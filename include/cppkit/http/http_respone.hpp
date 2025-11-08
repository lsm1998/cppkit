#pragma once

#include "http_request.hpp"

namespace cppkit::http
{
  class HttpResponse
  {
    friend HttpClient;

    int statusCode = 0;
    std::map<std::string, std::string> headers;
    std::vector<uint8_t> body;

  public:
    HttpResponse() = default;

    HttpResponse(const int statusCode, std::map<std::string, std::string> headers, std::vector<uint8_t> body)
      : statusCode(statusCode), headers(std::move(headers)), body(std::move(body))
    {
    }

    [[nodiscard]] int getStatusCode() const;

    [[nodiscard]] std::vector<uint8_t> getBody() const;

    [[nodiscard]] std::map<std::string, std::string> getHeaders() const;

    [[nodiscard]] std::string getHeader(const std::string& key) const;

  private:
    static HttpResponse parseResponse(const std::vector<uint8_t>& raw);
  };

  class HttpResponseWriter
  {
  public:
    explicit HttpResponseWriter(const int fd) : fd(fd)
    {
    }

    void setStatusCode(int statusCode);

    void setHeaders(const std::map<std::string, std::string>& headers);

    void setHeader(const std::string& key, const std::string& value);

    ssize_t write(const std::string& body);

    ssize_t write(const std::vector<uint8_t>& body);

  private:
    int fd{};
    int statusCode{HTTP_OK};
    std::map<std::string, std::string> headers;
  };
} // namespace cppkit::http