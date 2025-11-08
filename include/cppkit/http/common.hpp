#pragma once

namespace cppkit::http
{
  constexpr int BUFFER_SIZE = 4096;

  class HttpClient;

  class HttpServer;

  constexpr  int HTTP_OK = 200;
  constexpr int HTTP_BAD_REQUEST = 400;
  constexpr int HTTP_NOT_FOUND = 404;
  constexpr int HTTP_INTERNAL_SERVER_ERROR = 500;
} // namespace cppkit::http