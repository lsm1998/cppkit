#include "cppkit/http/server/http_server.hpp"
#include "cppkit/event/server.hpp"
#include "cppkit/strings.hpp"
#include "cppkit/platform.hpp"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <unistd.h>

namespace cppkit::http::server
{
    static const std::unordered_map<std::string, std::string> mimeTypes = {
        {".html", "text/html"}, {".htm", "text/html"},
        {".css", "text/css"}, {".js", "text/javascript"},
        {".json", "application/json"},
        {".png", "image/png"}, {".jpg", "image/jpeg"}, {".jpeg", "image/jpeg"},
        {".gif", "image/gif"}, {".ico", "image/x-icon"},
        {".svg", "image/svg+xml"}, {".txt", "text/plain"},
        {".pdf", "application/pdf"}, {".xml", "application/xml"}
    };

    void HttpServer::start()
    {
        this->_server = event::TcpServer(&this->_loop, this->_host, static_cast<uint16_t>(this->_port));

        this->_server.setReadable([this](const event::ConnInfo& conn)
        {
            HttpResponseWriter writer(conn.getFd());

            // 解析HTTP请求
            const HttpRequest request = HttpRequest::parse(conn.getFd());

            // 处理请求
            handleRequest(request, writer, conn.getFd());
            return 0;
        });
        this->_server.start();
        std::cout << "Started http server on " << this->_host << ":" << this->_port << std::endl;
        this->_loop.run();
    }

    void HttpServer::stop()
    {
        this->_loop.stop();
        this->_server.stop();
    }

    void HttpServer::Get(const std::string& path, const HttpHandler& handler) const
    {
        this->addRoute(HttpMethod::Get, path, handler);
    }

    void HttpServer::Post(const std::string& path, const HttpHandler& handler) const
    {
        this->addRoute(HttpMethod::Post, path, handler);
    }

    void HttpServer::Put(const std::string& path, const HttpHandler& handler) const
    {
        this->addRoute(HttpMethod::Put, path, handler);
    }

    void HttpServer::Delete(const std::string& path, const HttpHandler& handler) const
    {
        this->addRoute(HttpMethod::Delete, path, handler);
    }

    void HttpServer::setMaxFileSize(const uintmax_t size)
    {
        if (size <= 0)
        {
            throw std::invalid_argument("Max file size must be greater than 0");
        }
        _maxFileSize = size;
    }

    void HttpServer::addMiddleware(const std::shared_ptr<HttpMiddleware>& middleware)
    {
        _middlewares.push_back(middleware);
        std::ranges::sort(_middlewares, [](const auto& a, const auto& b)
        {
            return a->getPriority() > b->getPriority();
        });
    }

    void HttpServer::setStaticDir(const std::string_view path, const std::string_view dir)
    {
        _staticPath = path;
        _staticDir = dir;
        if (_staticPath.starts_with("/"))
        {
            _staticPath = _staticPath.substr(1);
        }
    }

    void HttpServer::addRoute(const HttpMethod method, const std::string& path, const HttpHandler& handler) const
    {
        // 判断路由是否已存在
        if (this->_router.exists(method, path))
        {
            throw std::runtime_error("Route already exists: " + httpMethodValue(method) + " " + path);
        }
        _router.addRoute(method, path, handler);
        std::cout << "Added route: " << httpMethodValue(method) << " " << path << std::endl;
    }

    void HttpServer::handleRequest(const HttpRequest& request, HttpResponseWriter& writer, int writerFd) const
    {
        std::unordered_map<std::string, std::string> params;
        const auto handler = _router.find(request.getMethod(), request.getPath(), params);
        if (handler == nullptr)
        {
            if (const auto isStatic = staticHandler(request, writer, writerFd); !isStatic)
            {
                writer.setStatusCode(HTTP_NOT_FOUND);
                writer.setHeader("Content-Type", "text/plain");
                writer.write("404 Not Found");
            }
            return;
        }
        request.setParams(params);

        // 处理中间件
        for (const auto& middleware : _middlewares)
        {
            // 路径匹配
            const std::string mwPath = middleware->getPath();
            if (mwPath != "/" && !startsWith(request.getPath(), mwPath))
            {
                continue;
            }
            if (!middleware->preProcess(request, writer))
            {
                return; // 中断请求处理
            }
        }

        // 调用路由处理函数
        handler(request, writer);

        // 后置处理中间件
        for (const auto& middleware : _middlewares)
        {
            const std::string mwPath = middleware->getPath();
            if (mwPath != "/" && !startsWith(request.getPath(), mwPath))
            {
                continue;
            }
            middleware->postProcess(request, writer);
        }
    }

    bool HttpServer::staticHandler(const HttpRequest& request, HttpResponseWriter& writer, int writerFd) const
    {
        if (_staticDir.empty() ||
            (request.getMethod() != HttpMethod::Get && request.getMethod() != HttpMethod::Head))
        {
            return false;
        }

        try
        {
            // 获取静态目录的绝对路径
            std::error_code ec;
            std::filesystem::path baseDir = std::filesystem::canonical(_staticDir, ec);
            if (ec) // 目录不存在
            {
                return false;
            }

            std::string reqPathStr = request.getPath();
            if (!reqPathStr.empty() && reqPathStr[0] == '/')
            {
                reqPathStr = reqPathStr.substr(1);
            }

            // 去除URL前缀
            if (!startsWith(reqPathStr, _staticPath))
            {
                return false;
            }
            reqPathStr = reqPathStr.substr(_staticPath.length());
            // 去除开头的斜杠
            if (!reqPathStr.empty() && reqPathStr[0] == '/')
            {
                reqPathStr = reqPathStr.substr(1);
            }

            // 拼接path
            std::filesystem::path targetPath = baseDir / reqPathStr;
            if (std::filesystem::exists(targetPath) && std::filesystem::is_directory(targetPath))
            {
                targetPath /= "index.html";
            }
            auto canonicalPath = std::filesystem::canonical(targetPath, ec);
            if (ec) // 文件不存在
            {
                return false;
            }

            std::string p1 = canonicalPath.string();

            if (std::string p2 = baseDir.string(); p1.find(p2) != 0)
            {
                writer.setStatusCode(HTTP_FORBIDDEN);
                writer.write("403 Forbidden");
                return true;
            }

            if (!std::filesystem::exists(canonicalPath) || !std::filesystem::is_regular_file(canonicalPath))
            {
                return false;
            }


            // 获取文件扩展名
            std::string ext = canonicalPath.extension().string();

            // 统一小写
            std::ranges::transform(ext, ext.begin(), [](const unsigned char c) { return std::tolower(c); });


            // 默认二进制流
            std::string contentType = "application/octet-stream";
            if (auto it = mimeTypes.find(ext); it != mimeTypes.end())
            {
                contentType = it->second;
            }
            auto fileSize = std::filesystem::file_size(canonicalPath);
            if (constexpr uintmax_t MAX_FILE_SIZE = 50 * 1024 * 1024; fileSize > MAX_FILE_SIZE)
            {
                writer.setStatusCode(HTTP_PAYLOAD_TOO_LARGE);
                writer.write("File too large to serve directly");
                return true;
            }

            writer.setStatusCode(HTTP_OK);
            writer.setHeader("Content-Type", contentType);
            writer.setHeader("Content-Length", std::to_string(fileSize));
            if (request.getMethod() == HttpMethod::Head)
            {
                return true;
            }
            if (std::ifstream file(canonicalPath, std::ios::binary); file.is_open())
            {
                //  读取内容
                std::vector<uint8_t> buffer(fileSize);
                if (file.read(reinterpret_cast<char*>(buffer.data()), static_cast<long>(fileSize)))
                {
                    writer.write(buffer);
                }
            }
        }
        catch (...)
        {
            // 获取错误信息
            writer.setStatusCode(HTTP_INTERNAL_SERVER_ERROR);
            writer.write("500 Internal Server Error");
            std::cerr << "Error serving static file for request: " << request.getPath() << std::endl;
            return false;
        }
        return false;
    }

    size_t HttpServer::sendFile(const int fd, const std::filesystem::path& filePath, const uintmax_t fileSize)
    {
        size_t totalBytesSent = 0;
        if constexpr (isApplePlatform)
        {
        }
        else if constexpr (isLinuxPlatform)
        {
        }
        else
        {
            if (std::ifstream file(filePath, std::ios::binary); file.is_open())
            {
                //  读取内容
                std::vector<uint8_t> buffer(fileSize);
                if (file.read(reinterpret_cast<char*>(buffer.data()), static_cast<long>(fileSize)))
                {
                    totalBytesSent += write(fd, buffer.data(), fileSize);
                }
            }
        }
        return totalBytesSent;
    }
} // namespace cppkit::http
