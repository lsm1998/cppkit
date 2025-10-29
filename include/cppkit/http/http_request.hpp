#pragma once

#include "common.hpp"
#include <cstdint>
#include <map>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace cppkit::http
{

  enum HttpMethod
  {
    Get,
    Post,
    Put,
    Delete
  };

  static const std::unordered_map<HttpMethod, std::string> _httpMethodValMap = {
      {HttpMethod::Get, "GET"},
      {HttpMethod::Post, "POST"},
      {HttpMethod::Put, "PUT"},
      {HttpMethod::Delete, "DELETE"},
  };

  static std::string httpMethodValue(HttpMethod method)
  {
    return _httpMethodValMap.at(method);
  }

  class HttpRequest
  {
    friend HttpClient;

  public:
    HttpMethod method;
    std::string url;
    std::map<std::string, std::string> headers;
    std::vector<uint8_t> body;

    HttpRequest() = default;

    HttpRequest(HttpMethod method, std::string url) : method(method), url(std::move(url)) {}

    HttpRequest(HttpMethod method, std::string url, std::map<std::string, std::string> headers)
        : method(method), url(std::move(url)), headers(std::move(headers))
    {
    }

    HttpRequest(
        HttpMethod method, std::string url, std::map<std::string, std::string> headers, std::vector<uint8_t> body)
        : method(method), url(std::move(url)), headers(std::move(headers)), body(std::move(body))
    {
    }

    void setHeader(const std::string& key, const std::string& value) { headers[key] = value; }

    void setHeaders(const std::map<std::string, std::string>& headers)
    {
      for (const auto& [key, value] : headers)
      {
        this->headers.insert_or_assign(key, value);
      }
    }

    void setHeaders(std::map<std::string, std::string>&& new_headers) noexcept { headers = std::move(new_headers); }

    void setBody(const std::vector<uint8_t>& data, const std::string& content_type = "")
    {
      this->body = data;
      if (!content_type.empty())
      {
        this->setContentType(content_type);
      }
      headers["Content-Length"] = std::to_string(body.size());
    }

    void setBody(const std::string& text, const std::string& content_type = "text/plain")
    {
      body.assign(text.begin(), text.end());
      headers["Content-Type"]   = content_type;
      headers["Content-Length"] = std::to_string(body.size());
    }

    void setContentType(const std::string& content_type) { this->headers["Content-Type"] = content_type; }

    void addQueryParam(const std::string& key, const std::string& value)
    {
      size_t qpos = url.find('?');
      if (qpos == std::string::npos)
      {
        url += "?" + key + "=" + value;
      }
      else
      {
        url += "&" + key + "=" + value;
      }
    }

    std::string getPath() const
    {
      size_t qpos = url.find('?');
      if (qpos == std::string::npos)
      {
        return this->url;
      }
      else
      {
        return this->url.substr(0, qpos);
      }
    }

    std::map<std::string, std::string> getQueryParams() const
    {
      size_t qpos = url.find('?');
      if (qpos == std::string::npos)
      {
        return {};
      }
      std::map<std::string, std::string> query_params;
      std::string query_str = url.substr(qpos + 1);
      std::istringstream query_stream(query_str);
      std::string pair;
      while (std::getline(query_stream, pair, '&'))
      {
        size_t eqpos = pair.find('=');
        if (eqpos != std::string::npos)
        {
          std::string key   = pair.substr(0, eqpos);
          std::string value = pair.substr(eqpos + 1);
          query_params[key] = value;
        }
      }
      return query_params;
    }

  private:
    std::vector<uint8_t> buildRequestData(const std::string& host, const std::string& path, int port, bool https) const;
  };

  class HttpResponse
  {
    friend HttpClient;

  private:
    int statusCode = 0;
    std::map<std::string, std::string> headers;
    std::vector<uint8_t> body;

  public:
    HttpResponse() = default;

    HttpResponse(int statusCode, std::map<std::string, std::string> headers, std::vector<uint8_t> body)
        : statusCode(statusCode), headers(std::move(headers)), body(std::move(body))
    {
    }

    int getStatusCode() const;

    std::vector<uint8_t> getBody() const;

    std::map<std::string, std::string> getHeaders() const;

    std::string getHeader(const std::string& key) const;

  private:
    static HttpResponse parseResponse(const std::vector<uint8_t>& raw);
  };

}  // namespace cppkit::http