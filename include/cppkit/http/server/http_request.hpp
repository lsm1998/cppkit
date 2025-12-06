#pragma once

#include "cppkit/http/http_request.hpp"
#include <vector>

namespace cppkit::http::server
{
  class HttpRequest
  {
    HttpMethod method{};
    std::string path;
    std::map<std::string, std::string> params;
    std::map<std::string, std::vector<std::string>> query;
    std::map<std::string, std::vector<std::string>> headers;
    mutable std::map<std::string, std::vector<std::string>> formData;
    int _fd{};
    std::string extraData{};

  public:
    explicit HttpRequest(const int fd) : _fd(fd)
    {
    }

    static HttpRequest parse(int fd);

    static HttpRequest parse(int fd, const std::string& raw, const std::string& extra_data);

    // 获取请求方法
    [[nodiscard]]
    HttpMethod getMethod() const;

    // 读取请求体
    [[nodiscard]]
    std::vector<u_int8_t> readBody() const;

    // 解析表单数据
    void parseFormData() const;

    // 获取请求路径
    [[nodiscard]]
    std::string getPath() const;

    // 获取url param
    [[nodiscard]]
    std::string getParam(const std::string& key) const;

    // 获取请求头
    [[nodiscard]]
    std::string getHeader(const std::string& key) const;

    // 获取所有请求头
    [[nodiscard]]
    std::map<std::string, std::vector<std::string>> getHeaders() const;

    // 获取查询参数
    [[nodiscard]]
    std::string getQuery(const std::string& key) const;

    // 获取所有查询参数
    [[nodiscard]]
    std::map<std::string, std::vector<std::string>> getQuerys() const;
  };
} // namespace cppkit::http::server