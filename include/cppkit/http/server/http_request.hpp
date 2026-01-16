#pragma once

#include "cppkit/http/http_request.hpp"
#include <vector>
#include <unordered_map>

namespace cppkit::http::server
{
    class HttpRequest
    {
        friend class HttpServer;
        friend class HttpContext;
        HttpMethod method{};
        std::string path;
        mutable std::unordered_map<std::string, std::string> _params{};
        std::map<std::string, std::vector<std::string>> query{};
        std::map<std::string, std::vector<std::string>> headers{};
        mutable std::map<std::string, std::vector<std::string>> formData;
        int _fd{};
        std::vector<u_int8_t> extraData{};
        mutable bool readBodyFlag{false};
        mutable std::vector<u_int8_t> _body{};
        mutable std::string _tempFilePath;  // 临时文件路径（大文件上传）

    public:
        explicit HttpRequest(const int fd) : _fd(fd)
        {
        }

        static HttpRequest parse(int fd, const std::string& raw, const std::string& extra_data);

        // 获取请求方法
        [[nodiscard]]
        HttpMethod getMethod() const;

        // 读取请求体
        [[nodiscard]]
        std::vector<u_int8_t> readBody() const;

        size_t readBody(char* buffer, size_t maxLen, size_t offset = 0) const;

        void resetBody(const std::vector<uint8_t>& body) const;

        // 解析表单数据
        void parseFormData() const;

        // 获取请求路径
        [[nodiscard]]
        std::string getPath() const;

        // 获取url param
        [[nodiscard]]
        std::string getParam(const std::string& key) const;

        // 获取所有url param
        [[nodiscard]]
        std::unordered_map<std::string, std::string> getParams() const;

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

        void setQuerys(const std::map<std::string, std::vector<std::string>>& querys);

        void setHeaders(const std::map<std::string, std::vector<std::string>>& headers);

        void setQuery(const std::string& key, const std::vector<std::string>& values);

        void setQuery(const std::string& key, const std::string& value);

        void addQuery(const std::string& key, const std::string& value);

        void setHeader(const std::string& key, const std::string& value);

        void addHeader(const std::string& key, const std::string& value);

        void setHeader(const std::string& key, const std::vector<std::string>& values);

        void appendBody(const char* data, size_t len) const;

        // 获取临时文件路径（大文件上传）
        [[nodiscard]]
        std::string getTempFilePath() const { return _tempFilePath; }

        // 检查 body 是否在内存中
        [[nodiscard]]
        bool hasBodyInMemory() const { return readBodyFlag && _tempFilePath.empty(); }

        // 检查是否使用临时文件
        [[nodiscard]]
        bool hasBodyInTempFile() const { return !_tempFilePath.empty(); }

    private:
        // 设置url param
        void setParams(std::unordered_map<std::string, std::string> params) const;

        // 设置临时文件路径（内部使用）
        void setTempFilePath(const std::string& path) const { _tempFilePath = path; }
    };
} // namespace cppkit::http::server
