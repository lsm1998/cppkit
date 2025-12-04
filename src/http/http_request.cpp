#include "cppkit/http/http_request.hpp"
#include <sstream>

using namespace cppkit::http;

HttpRequest HttpRequest::parse(const std::string& data)
{
    HttpRequest req;

    std::istringstream stream(data);
    std::string line;

    // 解析请求行
    if (std::getline(stream, line) && !line.empty())
    {
        // 移除可能的\r
        if (!line.empty() && line.back() == '\r')
            line.pop_back();

        std::istringstream req_line(line);
        std::string method_str, uri, version;

        if (req_line >> method_str >> uri >> version)
        {
            // 转换方法
            if (method_str == "GET")
                req.method = HttpMethod::Get;
            else if (method_str == "POST")
                req.method = HttpMethod::Post;
            else if (method_str == "PUT")
                req.method = HttpMethod::Put;
            else if (method_str == "DELETE")
                req.method = HttpMethod::Delete;

            req.url = std::move(uri);
        }
    }

    // 解析头部
    while (std::getline(stream, line) && line != "\r" && !line.empty())
    {
        if (!line.empty() && line.back() == '\r')
            line.pop_back();

        auto colon_pos = line.find(':');
        if (colon_pos != std::string::npos)
        {
            std::string key = line.substr(0, colon_pos);
            std::string value = line.substr(colon_pos + 1);

            // 移除前导空格
            size_t value_start = value.find_first_not_of(" \t");
            if (value_start != std::string::npos)
                value = value.substr(value_start);

            req.headers.emplace(std::move(key), std::move(value));
        }
    }

    // 解析正文 - 读取剩余内容
    std::string body_content;
    std::stringstream body_stream;

    // 剩余内容都是正文
    if (stream.rdbuf())
    {
        body_stream << stream.rdbuf();
        body_content = body_stream.str();

        // 如果有Content-Length头部，确保只读取指定长度
        auto content_length_it = req.headers.find("Content-Length");
        if (content_length_it != req.headers.end())
        {
            try
            {
                size_t content_length = std::stoull(content_length_it->second);
                body_content = body_content.substr(0, content_length);
            }
            catch (...)
            {
                // 解析失败，使用全部内容
            }
        }

        req.body.assign(body_content.begin(), body_content.end());
    }

    return req;
}

HttpRequest HttpRequest::parse(const std::vector<uint8_t>& data)
{
    return parse(std::string(data.begin(), data.end()));
}