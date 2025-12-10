#pragma once

#include "http_request.hpp"
#include <string>

#include "cppkit/websocket/client.hpp"


namespace cppkit::http
{
    class HttpResponse
    {
        friend HttpClient;
        friend websocket::WebSocketClient;

        int statusCode = 0;
        std::map<std::string, std::string> headers;
        std::vector<uint8_t> body;

    public:
        HttpResponse() = default;

        HttpResponse(const int statusCode, std::map<std::string, std::string> headers, std::vector<uint8_t> body)
            : statusCode(statusCode), headers(std::move(headers)), body(std::move(body))
        {
        }

        [[nodiscard]] int getStatusCode() const;

        [[nodiscard]] std::vector<uint8_t> getBody() const;

        [[nodiscard]] std::map<std::string, std::string> getHeaders() const;

        [[nodiscard]] std::string getHeader(const std::string& key) const;

    private:
        static HttpResponse parse(const std::vector<uint8_t>& raw);
    };
} // namespace cppkit::http
