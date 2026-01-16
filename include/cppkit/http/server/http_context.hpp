#pragma once
#include <string>
#include <vector>
#include <unistd.h>
#include <fcntl.h>
#include <cerrno>
#include <cstdio>
#include <filesystem>
#include "http_request.hpp"

namespace cppkit::http::server
{
    enum class ParseStatus
    {
        HeaderComplete, // header 解析完毕
        BodyComplete,   // body 也解析完毕
        Incomplete,     // 数据接收中
        Error
    };

    class HttpContext
    {
    public:
        // 配置：超过此大小使用临时文件（默认 10MB）
        static constexpr size_t BODY_MEMORY_THRESHOLD = 10 * 1024 * 1024;

        std::string recvBuffer;                    // 用于接收 Header 的缓冲区
        std::unique_ptr<HttpRequest> request;      // 解析后的请求对象
        std::vector<uint8_t> bodyBuffer;           // Body 缓冲区（小文件）
        std::string tempFilePath;                  // 临时文件路径（大文件）
        int tempFileFd = -1;                       // 临时文件描述符
        size_t contentLength = 0;                  // Content-Length
        bool headerParsed = false;                 // Header 是否已解析
        bool useTemporaryFile = false;             // 是否使用临时文件

        ~HttpContext()
        {
            if (tempFileFd >= 0)
            {
                close(tempFileFd);
                tempFileFd = -1;
            }
            if (!tempFilePath.empty() && std::filesystem::exists(tempFilePath))
            {
                std::filesystem::remove(tempFilePath);
            }
        }

        ParseStatus parse(const int fd)
        {
            char buffer[DEFAULT_BUFFER_SIZE];

            // 解析 Header
            if (!headerParsed)
            {
                while (true)
                {
                    const ssize_t len = read(fd, buffer, sizeof(buffer));

                    if (len < 0)
                    {
                        if (errno == EAGAIN || errno == EWOULDBLOCK) return ParseStatus::Incomplete;
                        return ParseStatus::Error;
                    }
                    if (len == 0) return ParseStatus::Error; // 对端关闭

                    recvBuffer.append(buffer, len);

                    // 检查 Header 结束标志
                    constexpr std::string_view delimiter = "\r\n\r\n";
                    if (const size_t pos = recvBuffer.find(delimiter); pos != std::string::npos)
                    {
                        const size_t headerEnd = pos + delimiter.size();
                        const std::string headerRaw = recvBuffer.substr(0, headerEnd);
                        const std::string extraData = recvBuffer.substr(headerEnd);

                        request = std::make_unique<HttpRequest>(HttpRequest::parse(fd, headerRaw, extraData));
                        recvBuffer.clear();
                        headerParsed = true;

                        // 获取 Content-Length
                        try
                        {
                            if (const std::string lenStr = request->getHeader("Content-Length"); !lenStr.empty())
                            {
                                contentLength = std::stoul(lenStr);
                            }
                        }
                        catch (...) { contentLength = 0; }

                        // 防止过大的 body
                        if (contentLength > MAX_BODY_SIZE)
                        {
                            return ParseStatus::Error;
                        }

                        // 如果没有 body，直接完成
                        if (contentLength == 0)
                        {
                            return ParseStatus::BodyComplete;
                        }

                        // 判断使用内存还是临时文件
                        if (contentLength > BODY_MEMORY_THRESHOLD)
                        {
                            // 大文件：使用临时文件
                            useTemporaryFile = true;

                            // 创建临时文件
                            char tmpTemplate[] = "/tmp/cppkit_upload_XXXXXX";
                            tempFileFd = mkstemp(tmpTemplate);
                            if (tempFileFd < 0)
                            {
                                return ParseStatus::Error;
                            }
                            tempFilePath = tmpTemplate;

                            // 将 extraData 写入临时文件
                            if (!extraData.empty())
                            {
                                if (write(tempFileFd, extraData.data(), extraData.size()) < 0)
                                {
                                    return ParseStatus::Error;
                                }
                            }
                        }
                        else
                        {
                            // 小文件：使用内存缓冲区
                            bodyBuffer.reserve(contentLength);
                            bodyBuffer.insert(bodyBuffer.end(), extraData.begin(), extraData.end());
                        }

                        // 如果 extraData 已经包含全部 body
                        const size_t receivedSize = useTemporaryFile ? extraData.size() : bodyBuffer.size();
                        if (receivedSize >= contentLength)
                        {
                            // 已经接收完整
                            if (useTemporaryFile)
                            {
                                request->setTempFilePath(tempFilePath);
                                close(tempFileFd);
                                tempFileFd = -1;
                            }
                            else
                            {
                                bodyBuffer.resize(contentLength);
                                request->resetBody(bodyBuffer);
                            }
                            return ParseStatus::BodyComplete;
                        }

                        break;
                    }
                }
            }

            // 读取 Body（如果需要）
            if (headerParsed)
            {
                const size_t receivedSize = useTemporaryFile ?
                    (tempFileFd >= 0 ? lseek(tempFileFd, 0, SEEK_CUR) : 0) : bodyBuffer.size();

                while (receivedSize < contentLength)
                {
                    const size_t remaining = contentLength - receivedSize;
                    const size_t toRead = std::min(remaining, static_cast<size_t>(DEFAULT_BUFFER_SIZE));
                    const ssize_t len = read(fd, buffer, toRead);

                    if (len < 0)
                    {
                        if (errno == EAGAIN || errno == EWOULDBLOCK)
                        {
                            return ParseStatus::Incomplete;
                        }
                        return ParseStatus::Error;
                    }
                    if (len == 0) return ParseStatus::Error; // 对端关闭

                    // 根据策略写入内存或文件
                    if (useTemporaryFile)
                    {
                        if (write(tempFileFd, buffer, len) < 0)
                        {
                            return ParseStatus::Error;
                        }
                    }
                    else
                    {
                        bodyBuffer.insert(bodyBuffer.end(), buffer, buffer + len);
                    }
                }

                // Body 读取完成
                if (useTemporaryFile)
                {
                    request->setTempFilePath(tempFilePath);
                    close(tempFileFd);
                    tempFileFd = -1;
                }
                else
                {
                    request->resetBody(bodyBuffer);
                }
                return ParseStatus::BodyComplete;
            }

            return ParseStatus::BodyComplete;
        }
    };
}
