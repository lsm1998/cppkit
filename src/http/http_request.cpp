#include "cppkit/http/http_request.hpp"
#include <string>

using namespace cppkit::http;

int HttpResponse::getStatusCode() const { return this->status_code; }

std::vector<uint8_t> HttpResponse::getBody() const { return this->body; }

std::map<std::string, std::string> HttpResponse::getHeaders() const {
  return this->headers;
}

std::string HttpResponse::getHeader(const std::string &key) const {
  return this->headers.at(key);
}

std::vector<uint8_t> HttpRequest::buildRequestData(const std::string &host,
                                                   const std::string &path,
                                                   int port, bool https) {

  std::ostringstream req;
  req << httpMethodValue(this->method) << " " << path << " HTTP/1.1\r\n";

  if (headers.find("Host") == headers.end()) {
    if ((https && port != 443) || (!https && port != 80))
      req << "Host: " << host << ":" << port << "\r\n";
    else
      req << "Host: " << host << "\r\n";
  }

  if (headers.find("Connection") == headers.end()) {
    req << "Connection: close\r\n";
  }

  for (const auto &h : headers) {
    req << h.first << ": " << h.second << "\r\n";
  }

  if (!body.empty()) {
    if (headers.find("Content-Length") == headers.end())
      req << "Content-Length: " << body.size() << "\r\n";
  }

  req << "\r\n";

  std::string headerStr = req.str();
  std::vector<uint8_t> data(headerStr.begin(), headerStr.end());
  data.insert(data.end(), body.begin(), body.end());
  return data;
}