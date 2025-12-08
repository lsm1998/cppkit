#pragma once

#include <string>
#include <unordered_map>

namespace cppkit::http
{
  constexpr int HTTP_CONTINUE = 100;
  constexpr int HTTP_SWITCHING_PROTOCOLS = 101;
  constexpr int HTTP_PROCESSING = 102;
  constexpr int HTTP_OK = 200;
  constexpr int HTTP_CREATED = 201;
  constexpr int HTTP_ACCEPTED = 202;
  constexpr int HTTP_NO_CONTENT = 203;
  constexpr int HTTP_BAD_REQUEST = 400;
  constexpr int HTTP_UNAUTHORIZED = 401;
  constexpr int HTTP_FORBIDDEN = 403;
  constexpr int HTTP_NOT_FOUND = 404;
  constexpr int HTTP_METHOD_NOT_ALLOWED = 405;
  constexpr int HTTP_INTERNAL_SERVER_ERROR = 500;
  constexpr int HTTP_NOT_IMPLEMENTED = 501;
  constexpr int HTTP_BAD_GEOMETRY = 502;

  // 定义HTTP状态码映射
  static std::unordered_map<int, std::string> HTTP_STATUS_MAP = {
      {HTTP_CONTINUE, "Continue"},
      {HTTP_SWITCHING_PROTOCOLS, "Switching Protocols"},
      {HTTP_PROCESSING, "Processing"},
      {HTTP_OK, "OK"},
      {HTTP_CREATED, "Created"},
      {HTTP_ACCEPTED, "Accepted"},
      {HTTP_NO_CONTENT, "No Content"},
      {HTTP_BAD_REQUEST, "Bad Request"},
      {HTTP_UNAUTHORIZED, "Unauthorized"},
      {HTTP_FORBIDDEN, "Forbidden"},
      {HTTP_NOT_FOUND, "Not Found"},
      {HTTP_METHOD_NOT_ALLOWED, "Method Not Allowed"},
      {HTTP_INTERNAL_SERVER_ERROR, "Internal Server Error"},
      {HTTP_NOT_IMPLEMENTED, "Not Implemented"},
      {HTTP_BAD_GEOMETRY, "Bad Gateway"},
  };
} // namespace cppkit::http