#include "cppkit/http/server/http_request.hpp"
#include <unistd.h>
#include <sstream>
#include <thread>

namespace cppkit::http::server
{
  HttpRequest HttpRequest::parse(int fd, const std::string& raw, const std::string& extra_data)
  {
    HttpRequest request{fd};
    request.extraData.reserve(extra_data.size());
    request.extraData.insert(request.extraData.end(), extra_data.begin(), extra_data.end());

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
    if (size_t qPos = request.path.find('?'); qPos != std::string::npos)
    {
      std::string query_string = request.path.substr(qPos + 1);
      request.path = request.path.substr(0, qPos);
      std::istringstream query_stream(query_string);
      std::string pair;
      while (std::getline(query_stream, pair, '&'))
      {
        if (size_t eqpos = pair.find('='); eqpos != std::string::npos)
        {
          std::string key = pair.substr(0, eqpos);
          std::string value = pair.substr(eqpos + 1);
          if (auto& it = request.query[key]; it.empty())
          {
            request.query[key] = {std::move(value)};
          }
          else
          {
            it.push_back(std::move(value));
          }
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
    if (readBodyFlag)
    {
      return _body;
    }

    size_t contentLength = 0;
    try
    {
      if (const std::string lenStr = getHeader("content-length"); !lenStr.empty())
      {
        contentLength = std::stoul(lenStr);
      }
    }
    catch (...)
    {
      contentLength = 0;
    }

    // 不可以超过内存限制
    if (contentLength > MAX_BODY_SIZE)
    {
      throw std::runtime_error("Request body too large.");
    }

    if (contentLength > 0)
    {
      _body.reserve(contentLength + extraData.size());
      _body.insert(_body.end(), extraData.begin(), extraData.end());
      size_t remaining = contentLength - extraData.size();
      char buffer[DEFAULT_BUFFER_SIZE];
      while (remaining > 0)
      {
        const ssize_t len = read(this->_fd, buffer, std::min(DEFAULT_BUFFER_SIZE, static_cast<int>(remaining)));
        if (len <= 0)
        {
          if (errno == EAGAIN || errno == EWOULDBLOCK)
          {
            // 使用readBody需要一次性读取完毕，遇到EAGAIN则让出时间片重试
            // 后续可以使用协程或者异步回调的方式优化
            std::this_thread::yield();
            continue;
          }
          throw std::runtime_error("Failed to read body from socket.");
        }
        _body.insert(_body.end(), buffer, buffer + len);
        remaining -= len;
      }
    }
    readBodyFlag = true;
    return _body;
  }

  size_t HttpRequest::readBody(char* buffer, const size_t maxLen, size_t offset) const
  {
    if (readBodyFlag) // 是否已经读取完毕了
    {
      if (offset >= _body.size())
        return 0;
      const size_t readLen = std::min(maxLen, _body.size() - offset);
      std::copy_n(_body.begin() + static_cast<long>(offset), readLen, buffer);
      return readLen;
    }

    size_t contentLength = 0;
    try
    {
      if (const std::string lenStr = getHeader("content-length"); !lenStr.empty())
      {
        contentLength = std::stoul(lenStr);
      }
    }
    catch (...)
    {
      return 0; // 解析失败或没有长度，无法处理流式读取
    }

    // 如果 offset 已经超过了总长度，说明读完了
    if (offset >= contentLength)
    {
      return 0;
    }

    size_t totalBytesCopied = 0;
    char* currentBufPtr = buffer;
    size_t currentMaxLen = maxLen;

    // 处理extraData数据
    if (offset < extraData.size())
    {
      // 计算extraData中可用的数据量
      const size_t availableInExtra = extraData.size() - offset;
      const size_t toCopy = std::min(currentMaxLen, availableInExtra);

      std::memcpy(currentBufPtr, extraData.data() + offset, toCopy);

      totalBytesCopied += toCopy;
      currentBufPtr += toCopy;
      currentMaxLen -= toCopy;

      // 更新offset
      offset += toCopy;
    }

    // 是否已经填满buffer
    if (currentMaxLen == 0)
    {
      return totalBytesCopied;
    }

    const size_t remainingTotal = contentLength - offset;

    // 计算还需要从socket读取的字节数
    const size_t bytesToReadFromSocket = std::min(currentMaxLen, remainingTotal);

    if (bytesToReadFromSocket > 0)
    {
      if (const ssize_t ret = read(this->_fd, currentBufPtr, bytesToReadFromSocket); ret > 0)
      {
        totalBytesCopied += ret;
      }
      else if (ret == 0) // EOF
      {
        return totalBytesCopied;
      }
      else
      {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
        {
          // 数据未准备好，返回已读取的字节数
          return totalBytesCopied;
        }
        throw std::runtime_error("Socket read error");
      }
    }

    return totalBytesCopied;
  }

  void HttpRequest::resetBody(const std::vector<uint8_t>& body) const
  {
    readBodyFlag = true;
    _body = body;
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

  std::unordered_map<std::string, std::string> HttpRequest::getParams() const
  {
    return _params;
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

  void HttpRequest::setQuerys(const std::map<std::string, std::vector<std::string>>& querys)
  {
    this->query = querys;
  }

  void HttpRequest::setHeaders(const std::map<std::string, std::vector<std::string>>& headers)
  {
    this->headers = headers;
  }

  void HttpRequest::setQuery(const std::string& key, const std::vector<std::string>& values)
  {
    this->query[key] = values;
  }

  void HttpRequest::setQuery(const std::string& key, const std::string& value)
  {
    this->query[key] = {value};
  }

  void HttpRequest::addQuery(const std::string& key, const std::string& value)
  {
    auto& it = this->query[key];
    it.push_back(value);
  }

  void HttpRequest::addHeader(const std::string& key, const std::string& value)
  {
    auto& it = this->headers[key];
    it.push_back(value);
  }

  void HttpRequest::setHeader(const std::string& key, const std::string& value)
  {
    this->headers[key] = {value};
  }

  void HttpRequest::setHeader(const std::string& key, const std::vector<std::string>& values)
  {
    this->headers[key] = values;
  }

  void HttpRequest::appendBody(const char* data, const size_t len) const
  {
    if (len == 0)
      return;
    _body.reserve(_body.size() + len);
    _body.insert(_body.end(), data, data + len);
    readBodyFlag = true;
  }

  void HttpRequest::setParams(std::unordered_map<std::string, std::string> params) const
  {
    this->_params = std::move(params);
  }
} // namespace cppkit::http::server
