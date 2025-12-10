#pragma once

#include "http_request.hpp"
#include "http_response.hpp"
#include "pool_connection.hpp"
#include <arpa/inet.h>
#include <map>
#include <string>
#include <vector>
#include <cstring>
#include <stdexcept>
#include <queue>
#include <mutex>
#include <chrono>
#include <condition_variable>

namespace cppkit::http
{
    class HttpClient
    {
        size_t timeoutSeconds{30}; // 超时时间

        size_t maxConnections{10}; // max connections in pool

        size_t connectionTimeout{30}; // connection timeout seconds

        std::map<std::string, std::queue<std::unique_ptr<PoolConnection>>> connectionPool; // 连接池

        std::mutex poolMutex; // 连接池互斥锁

        std::condition_variable poolCond; // 连接池条件变量

    public:
        HttpClient() = default;

        HttpClient(const HttpClient&) = delete;
        HttpClient& operator=(const HttpClient&) = delete;

        explicit HttpClient(const size_t timeoutSeconds) : timeoutSeconds(timeoutSeconds)
        {
        };

        HttpClient(const size_t timeoutSeconds, const size_t maxConnections)
            : timeoutSeconds(timeoutSeconds)
              , maxConnections(maxConnections)
        {
        };

        ~HttpClient();

        HttpResponse Do(const HttpRequest& request);

        HttpResponse Get(const std::string& url, const std::map<std::string, std::string>& headers = {});

        HttpResponse Post(const std::string& url,
                          const std::map<std::string, std::string>& headers = {},
                          const std::vector<uint8_t>& body = {});

        HttpResponse Delete(const std::string& url,
                            const std::map<std::string, std::string>& headers = {},
                            const std::vector<uint8_t>& body = {});

        HttpResponse Put(const std::string& url,
                         const std::map<std::string, std::string>& headers = {},
                         const std::vector<uint8_t>& body = {});

        void setMaxConnections(const size_t max) { maxConnections = max; }

        [[nodiscard]]
        size_t getMaxConnections() const { return maxConnections; }

    private:
        // 解析URL，提取主机、路径和端口
        static void parseUrl(const std::string& url, std::string& host, std::string& path, int& port, bool https);

        // 连接到主机，返回socket fd
        static int connect2host(const std::string& host, int port, size_t timeoutSeconds);

        // 发送数据直到全部发送完毕
        static size_t sendData(int fd, const std::vector<uint8_t>& body);

        // 接收数据直到连接关闭
        static std::vector<uint8_t> recvData(int fd);

        // 连接是否存活
        static bool isConnectionAlive(int fd);

        // 获取连接
        std::unique_ptr<PoolConnection> getConnection(const std::string& host, int port);

        // 归还连接到连接池
        void returnConnection(std::unique_ptr<PoolConnection> conn);

        // 清理长时间未使用的连接
        void cleanUpOldConnections();
    };
} // namespace cppkit::http
