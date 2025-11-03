#include "cppkit/http/http_router.hpp"
#include "cppkit/http/http_request.hpp"
#include "cppkit/strings.hpp"

namespace cppkit::http
{
  void Router::addRoute(const HttpMethod method, const std::string& path, const HttpHandler& handler) const
  {
    const auto parts = split(path, '/');
    RouteNode* node = root.get();

    for (const auto& part : parts)
    {
      std::string key = part;
      bool isParam = false, isWild = false;

      if (!part.empty() && part[0] == ':')
      {
        key = ":";
        isParam = true;
      }
      if (!part.empty() && part[0] == '*')
      {
        key = "*";
        isWild = true;
      }

      if (!node->children.contains(key))
      {
        node->children[key] = std::make_unique<RouteNode>();
        node->children[key]->segment = part;
        node->children[key]->isParam = isParam;
        node->children[key]->isWild = isWild;
      }
      node = node->children[key].get();
    }
    node->handlers[httpMethodValue(method)] = handler;
  }

  bool Router::exists(const HttpMethod method, const std::string& path) const
  {
    const auto parts = split(path, '/');
    std::unordered_map<std::string, std::string> params;
    RouteNode* node = match(root.get(), parts, 0, params);

    if (node && node->handlers.contains(httpMethodValue(method)))
    {
      return true;
    }
    return false;
  }

  RouteNode* Router::match(RouteNode* node,
      const std::vector<std::string>& parts,
      size_t index,
      std::unordered_map<std::string, std::string>& params) const
  {
    if (index == parts.size() || node->isWild)
    {
      return node;
    }

    const std::string& part = parts[index];

    if (node->children.contains(part))
    {
      RouteNode* child = node->children[part].get();
      RouteNode* result = match(child, parts, index + 1, params);
      if (result)
      {
        return result;
      }
    }

    for (const auto& [key, childPtr] : node->children)
    {
      RouteNode* child = childPtr.get();
      if (child->isParam)
      {
        params[child->segment.substr(1)] = part;
        RouteNode* result = match(child, parts, index + 1, params);
        if (result)
        {
          return result;
        }
        params.erase(child->segment.substr(1));
      }
      else if (child->isWild)
      {
        params[child->segment.substr(1)] = join(std::vector<std::string>(parts.begin() + index, parts.end()), "/");
        return child;
      }
    }
    return nullptr;
  }
} // namespace cppkit::http