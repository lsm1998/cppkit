#pragma once

#include "http_request.hpp"
#include "http_respone.hpp"
#include <string>
#include <unordered_map>
#include <functional>
#include <memory>

namespace cppkit::http
{
  using HttpHandler = std::function<void(const HttpRequest&, HttpResponseWriter&)>;

  struct RouteNode
  {
    std::string segment;
    bool isParam = false;
    bool isWild = false;
    std::unordered_map<std::string, std::unique_ptr<RouteNode>> children;
    std::unordered_map<std::string, HttpHandler> handlers;
  };

  class Router
  {
  public:
    Router() : root(std::make_unique<RouteNode>())
    {
    }

    void addRoute(HttpMethod method, const std::string& path, const HttpHandler& handler) const;

    [[nodiscard]] bool exists(HttpMethod method, const std::string& path) const;

    [[nodiscard]] HttpHandler find(HttpMethod method, const std::string& path) const;

  private:
    static RouteNode* match(RouteNode* node,
        const std::vector<std::string>& parts,
        size_t index,
        std::unordered_map<std::string, std::string>& params);

    std::unique_ptr<RouteNode> root;
  };
} // namespace cppkit::http