#pragma once

#include "http_request.hpp"
#include "http_response.hpp"

namespace cppkit::http::server
{
    class HttpMiddleware
    {
    public:
        HttpMiddleware() = default;

        virtual ~HttpMiddleware() = default;

        [[nodiscard]] virtual std::string getPath() const
        {
            return "/";
        }

        // 前置处理
        virtual bool preProcess(const HttpRequest& req, HttpResponseWriter& res) const
        {
            return true;
        }

        // 后置处理
        virtual void postProcess(const HttpRequest& req, HttpResponseWriter& res) const
        {
        }

        // 优先级，数值越大优先级越高
        [[nodiscard]] virtual int getPriority() const
        {
            return 0;
        }
    };
} // namespace cppkit::http::server
