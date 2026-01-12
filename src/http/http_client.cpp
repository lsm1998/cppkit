#include "cppkit/http/http_client.hpp"
#include "cppkit/strings.hpp"
#include <netdb.h>
#include <stdexcept>
#include <unistd.h>
#include <vector>
#include <fcntl.h>
#include <ranges>
#include <netinet/tcp.h>
#include <poll.h>
#include <cstring>
#include <algorithm>
#include <iostream>
#include <memory>

namespace cppkit::http
{
    HttpResponse HttpClient::Get(const std::string& url, const std::map<std::string, std::string>& headers)
    {
        const HttpRequest request(HttpMethod::Get, url, headers);
        return this->Do(request);
    }

    HttpResponse HttpClient::Post(
        const std::string& url,
        const std::map<std::string, std::string>& headers,
        const std::vector<uint8_t>& body)
    {
        const HttpRequest request(HttpMethod::Post, url, headers, body);
        return this->Do(request);
    }

    HttpResponse HttpClient::Put(
        const std::string& url,
        const std::map<std::string, std::string>& headers,
        const std::vector<uint8_t>& body)
    {
        const HttpRequest request(HttpMethod::Put, url, headers, body);
        return this->Do(request);
    }

    HttpResponse HttpClient::Delete(
        const std::string& url,
        const std::map<std::string, std::string>& headers,
        const std::vector<uint8_t>& body)
    {
        const HttpRequest request(HttpMethod::Delete, url, headers, body);
        return this->Do(request);
    }

    HttpResponse HttpClient::Do(const HttpRequest& request)
    {
        std::string host, path;
        int port{};
        const bool https = request.url.starts_with("https");

        if (https)
        {
            throw std::runtime_error("HTTPS is not supported yet");
        }

        // 解析 URL
        parseUrl(request.url, host, path, port, https);

        // 获取连接
        auto conn = getConnection(host, port);

        // 设置 TCP_NODELAY
        constexpr int flag = 1;
        setsockopt(conn->fd, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(int));

        // 构建并发送请求
        if (const auto requestData = request.build(host, path, port, https);
            sendData(conn->fd, requestData) <= 0)
        {
            throw std::runtime_error("Failed to send request to " + host);
        }

        // 接收响应
        const auto data = recvData(conn->fd);

        if (data.empty())
        {
            throw std::runtime_error("Empty response from " + host);
        }

        // 解析响应对象
        HttpResponse response = HttpResponse::parse(data);

        // 处理连接保持 (Keep-Alive)
        const std::string connHeader = response.getHeader("Connection");
        // HTTP/1.1 默认 Keep-Alive，除非 Connection: close
        bool keepAlive = true;

        if (startsWith(toUpper(connHeader), toUpper("close")))
        {
            keepAlive = false;
        }

        if (keepAlive)
        {
            returnConnection(std::move(conn));
        }
        return response;
    }

    void HttpClient::parseUrl(const std::string& url, std::string& host, std::string& path, int& port, bool https)
    {
        // 纯字符串处理，不要在这里做 DNS 解析，否则 Host 头会变成 IP
        const std::string prefix = https ? "https://" : "http://";

        size_t start = 0;
        if (url.starts_with(prefix))
        {
            start = prefix.size();
        }
        else if (url.starts_with("http://"))
        {
            // 处理这种不匹配的情况，或者默认 http
            start = 7;
        }

        const size_t slash = url.find('/', start);
        if (slash == std::string::npos)
        {
            host = url.substr(start);
            path = "/";
        }
        else
        {
            host = url.substr(start, slash - start);
            path = url.substr(slash);
        }

        if (const size_t colon = host.find(':'); colon != std::string::npos)
        {
            port = std::stoi(host.substr(colon + 1));
            host = host.substr(0, colon);
        }
        else
        {
            port = https ? 443 : 80;
        }
    }

    int HttpClient::connect2host(const std::string& host, const int port, const size_t timeoutSeconds)
    {
        struct addrinfo hints{}, *res = nullptr;
        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;

        const std::string port_str = std::to_string(port);

        // getaddrinfo 会自动处理域名到 IP 的转换，也会处理已经是 IP 字符串的情况
        if (getaddrinfo(host.c_str(), port_str.c_str(), &hints, &res) != 0)
        {
            return -1;
        }

        // 使用 unique_ptr 自动管理 freeaddrinfo
        std::unique_ptr<addrinfo, void(*)(addrinfo*)> addr_ptr(res, freeaddrinfo);

        int fd = -1;
        struct addrinfo* ptr = res;

        // 遍历所有可能的地址直到连接成功
        for (; ptr != nullptr; ptr = ptr->ai_next)
        {
            fd = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
            if (fd < 0) continue;

            // 设置非阻塞
            const int flags = fcntl(fd, F_GETFL, 0);
            if (flags == -1 || fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1)
            {
                close(fd);
                fd = -1;
                continue;
            }

            const int connectResult = connect(fd, ptr->ai_addr, ptr->ai_addrlen);
            if (connectResult == 0)
            {
                // 立即连接成功 (罕见，但在 localhost 可能发生)
                // 恢复阻塞模式
                fcntl(fd, F_SETFL, flags);
                break;
            }

            if (errno != EINPROGRESS)
            {
                close(fd);
                fd = -1;
                continue;
            }

            // 使用 poll 替代 select (避免 1024 fd 限制)
            struct pollfd pfd{};
            pfd.fd = fd;
            pfd.events = POLLOUT;

            const int pollResult = poll(&pfd, 1, static_cast<int>(timeoutSeconds * 1000));

            if (pollResult > 0)
            {
                int so_error = 0;
                socklen_t len = sizeof(so_error);
                if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &so_error, &len) == 0 && so_error == 0)
                {
                    // 连接成功，恢复阻塞模式
                    fcntl(fd, F_SETFL, flags);
                    break;
                }
            }

            // 超时或错误
            close(fd);
            fd = -1;
        }

        return fd;
    }

    size_t HttpClient::sendData(const int fd, const std::vector<uint8_t>& body)
    {
        size_t totalSent = 0;
        const size_t size = body.size();

        // 防止 SIGPIPE 信号导致进程退出
        const int flags = MSG_NOSIGNAL;

        while (totalSent < size)
        {
            ssize_t sent = send(fd, body.data() + totalSent, size - totalSent, flags);

            if (sent < 0)
            {
                if (errno == EINTR) continue;
                return -1;
            }
            if (sent == 0) break; // 对方关闭连接

            totalSent += sent;
        }
        return totalSent;
    }

    // 解析 HTTP 头部中的 Content-Length
    static ssize_t getContentLength(const std::string& headers)
    {
        // 简单的查找，实际应用可能需要更严谨的解析
        static const std::string key = "Content-Length:";
        auto it = std::search(headers.begin(), headers.end(),
                              key.begin(), key.end(),
                              [](char a, char b) { return tolower(a) == tolower(b); });

        if (it != headers.end())
        {
            size_t start = std::distance(headers.begin(), it) + key.size();
            size_t end = headers.find("\r\n", start);
            if (end != std::string::npos)
            {
                std::string val = headers.substr(start, end - start);
                try
                {
                    return std::stoll(val);
                }
                catch (...) { return -1; }
            }
        }
        return -1;
    }

    std::vector<uint8_t> HttpClient::recvData(const int fd)
    {
        std::vector<uint8_t> buffer;
        uint8_t temp[4096];
        bool headerFinished = false;
        size_t headerSize = 0;
        ssize_t contentLength = -1;

        // 1. 读取直到获取完整的 Header (\r\n\r\n)
        while (!headerFinished)
        {
            const ssize_t n = recv(fd, temp, sizeof(temp), 0);
            if (n <= 0) break; // 连接关闭或错误

            buffer.insert(buffer.end(), temp, temp + n);

            // 检查是否包含双换行
            const std::string_view view(reinterpret_cast<const char*>(buffer.data()), buffer.size());
            const size_t pos = view.find("\r\n\r\n");

            if (pos != std::string::npos)
            {
                headerFinished = true;
                headerSize = pos + 4;

                // 解析 Header 部分以获取 Body 长度
                std::string headerStr(view.substr(0, pos));
                contentLength = getContentLength(headerStr);
            }
        }

        if (!headerFinished)
        {
            // 还没读完头连接就断了，或者根本没数据
            return buffer;
        }

        // 根据 Content-Length 读取剩余 Body
        if (contentLength >= 0)
        {
            const size_t totalExpected = headerSize + contentLength;
            while (buffer.size() < totalExpected)
            {
                size_t needed = totalExpected - buffer.size();
                const size_t readSize = std::min(needed, sizeof(temp));

                const ssize_t n = recv(fd, temp, readSize, 0);
                if (n <= 0) break;
                buffer.insert(buffer.end(), temp, temp + n);
            }
        }
        else
        {
            while (true)
            {
                const ssize_t n = recv(fd, temp, sizeof(temp), 0);
                if (n <= 0) break;
                buffer.insert(buffer.end(), temp, temp + n);
            }
        }

        return buffer;
    }

    HttpClient::~HttpClient()
    {
        std::unique_lock lock(poolMutex);
        for (auto& pair : connectionPool)
        {
            // 清空队列
            std::queue<std::unique_ptr<PoolConnection>> empty;
            std::swap(pair.second, empty);
        }
    }

    bool HttpClient::isConnectionAlive(const int fd)
    {
        // 使用 poll 检查是否可读
        pollfd pfd{};
        pfd.fd = fd;
        pfd.events = POLLIN;

        if (poll(&pfd, 1, 0) > 0)
        {
            if (pfd.revents & POLLIN)
            {
                char buffer[1];
                const ssize_t n = recv(fd, buffer, 1, MSG_PEEK | MSG_DONTWAIT);
                if (n == 0) return false; // FIN received
                return false;
            }
            return false;
        }
        return true;
    }

    std::unique_ptr<PoolConnection> HttpClient::getConnection(const std::string& host, int port)
    {
        std::unique_lock lock(poolMutex);
        cleanUpOldConnections();

        const std::string key = host + ":" + std::to_string(port);

        // 尝试从池中复用
        if (connectionPool.contains(key))
        {
            auto& queue = connectionPool[key];
            while (!queue.empty())
            {
                auto conn = std::move(queue.front());
                queue.pop();

                if (isConnectionAlive(conn->fd))
                {
                    conn->lastUsed = std::chrono::steady_clock::now();
                    return conn;
                }
            }
        }

        // 检查最大连接数限制
        size_t idleConnections = 0;
        for (const auto& val : connectionPool | std::views::values)
        {
            idleConnections += val.size();
        }

        lock.unlock();

        int fd = connect2host(host, port, connectionTimeout);
        if (fd < 0)
        {
            throw std::runtime_error("Failed to connect to " + host + ":" + std::to_string(port));
        }

        return std::make_unique<PoolConnection>(fd, host, port);
    }

    void HttpClient::returnConnection(std::unique_ptr<PoolConnection> conn)
    {
        if (!conn || conn->fd == -1) return;

        std::unique_lock lock(poolMutex);
        conn->lastUsed = std::chrono::steady_clock::now();

        const std::string key = conn->host + ":" + std::to_string(conn->port);
        connectionPool[key].push(std::move(conn));

        poolCond.notify_one();
    }

    void HttpClient::cleanUpOldConnections()
    {
        const auto now = std::chrono::steady_clock::now();
        constexpr auto timeout = std::chrono::minutes(5);

        for (auto& val : connectionPool | std::views::values)
        {
            auto& queue = val;
            const size_t s = queue.size();
            for (size_t i = 0; i < s; ++i)
            {
                auto conn = std::move(queue.front());
                queue.pop();

                if (std::chrono::duration_cast<std::chrono::minutes>(now - conn->lastUsed) < timeout)
                {
                    queue.push(std::move(conn));
                }
            }
        }
    }
} // namespace cppkit::http
