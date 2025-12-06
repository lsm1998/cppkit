#include "cppkit/http/server/http_request.hpp"
#include <unistd.h>
#include <sstream>

constexpr int BUFFER_SIZE = 1024;

namespace cppkit::http::server
{
  HttpRequest HttpRequest::parse(const int fd)
  {
    HttpRequest request{fd};

    std::string raw_request;
    char buffer[BUFFER_SIZE];

    const std::string header_delimiter = "\r\n\r\n";

    while (true)
    {
      const ssize_t len = read(fd, buffer, BUFFER_SIZE);
      if (len <= 0)
      {
        // throw std::runtime_error("Failed to read from socket.");
        break;
      }

      raw_request.append(buffer, len);

      // 检查是否已经读取到头部结束标志
      if (raw_request.find(header_delimiter) != std::string::npos)
      {
        // 是否存在多读的数据
        const size_t header_end_pos = raw_request.find(header_delimiter) + header_delimiter.length();
        return parse(fd, raw_request, raw_request.substr(header_end_pos));
      }
    }
  }

  HttpRequest HttpRequest::parse(const int fd, const std::string& raw, const std::string& extra_data)
  {
    HttpRequest request{fd};
    request.extraData = extra_data;

    std::istringstream stream(raw);
    std::string line;

    // 解析请求行
    if (std::getline(stream, line) && !line.empty())
    {
      // 移除可能的\r
      if (!line.empty() && line.back() == '\r')
        line.pop_back();

      std::istringstream req_line(line);
      std::string uri, version;

      if (std::string method_str; req_line >> method_str >> uri >> version)
      {
        // 转换方法
        if (method_str == "GET")
          request.method = HttpMethod::Get;
        else if (method_str == "POST")
          request.method = HttpMethod::Post;
        else if (method_str == "PUT")
          request.method = HttpMethod::Put;
        else if (method_str == "DELETE")
          request.method = HttpMethod::Delete;

        request.path = std::move(uri);
      }
    }

    // 解析查询参数
    if (size_t qpos = request.path.find('?'); qpos != std::string::npos)
    {
      std::string query_string = request.path.substr(qpos + 1);
      request.path = request.path.substr(0, qpos);
      std::istringstream query_stream(query_string);
      std::string pair;
      while (std::getline(query_stream, pair, '&'))
      {
        if (size_t eqpos = pair.find('='); eqpos != std::string::npos)
        {
          std::string key = pair.substr(0, eqpos);
          std::string value = pair.substr(eqpos + 1);
          request.query.at(key).push_back(value);
        }
      }
    }

    // 解析头部
    while (std::getline(stream, line) && line != "\r" && !line.empty())
    {
      if (!line.empty() && line.back() == '\r')
        line.pop_back();

      if (auto colon_pos = line.find(':'); colon_pos != std::string::npos)
      {
        std::string key = line.substr(0, colon_pos);
        std::string value = line.substr(colon_pos + 1);

        // 移除前导空格
        if (size_t value_start = value.find_first_not_of(" \t"); value_start != std::string::npos)
          value = value.substr(value_start);

        if (auto& it = request.headers[key]; it.empty())
        {
          request.headers[key] = {std::move(value)};
        }
        else
        {
          it.push_back(std::move(value));
        }
      }
    }
    return request;
  }

  HttpMethod HttpRequest::getMethod() const
  {
    return method;
  }

  std::vector<u_int8_t> HttpRequest::readBody() const
  {
    std::vector<u_int8_t> body;
    if (const size_t contentLength = std::stoul(getHeader("Content-Length")); contentLength > 0)
    {
      body.insert(body.end(), extraData.begin(), extraData.end());
      size_t remaining = contentLength - extraData.size();
      char buffer[BUFFER_SIZE];
      while (remaining > 0)
      {
        const ssize_t len = read(this->_fd, buffer, std::min(BUFFER_SIZE, static_cast<int>(remaining)));
        if (len <= 0)
        {
          throw std::runtime_error("Failed to read body from socket.");
        }
        body.insert(body.end(), buffer, buffer + len);
        remaining -= len;
      }
    }
    return body;
  }

  void HttpRequest::parseFormData() const
  {
    // 获取请求格式
    if (const std::string contentType = getHeader("Content-Type");
      contentType.find("application/x-www-form-urlencoded") == std::string::npos)
    {
      return;
    }
    auto body = readBody();
    std::istringstream stream(std::string(body.begin(), body.end()));
    std::string pair;
    while (std::getline(stream, pair, '&'))
    {
      if (size_t eqpos = pair.find('='); eqpos != std::string::npos)
      {
        std::string key = pair.substr(0, eqpos);
        std::string value = pair.substr(eqpos + 1);
        formData.at(key).push_back(value);
      }
    }
  }

  std::string HttpRequest::getPath() const
  {
    return path;
  }

  std::string HttpRequest::getParam(const std::string& key) const
  {
    return _params.at(key);
  }

  std::string HttpRequest::getHeader(const std::string& key) const
  {
    if (const auto it = headers.find(key); it != headers.end() && !it->second.empty())
      return it->second[0];
    return "";
  }

  std::map<std::string, std::vector<std::string>> HttpRequest::getHeaders() const
  {
    return headers;
  }

  std::string HttpRequest::getQuery(const std::string& key) const
  {
    if (const auto it = query.find(key); it != query.end() && !it->second.empty())
      return it->second[0];
    return "";
  }

  std::map<std::string, std::vector<std::string>> HttpRequest::getQuerys() const
  {
    return query;
  }

  void HttpRequest::setParams(std::unordered_map<std::string, std::string> params) const
  {
    this->_params = std::move(params);
  }
}