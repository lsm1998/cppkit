#include "cppkit/http/http_client.hpp"

int main() {
  cppkit::http::HttpClient client{};
  client.Get("https://www.google.com/");
  return 0;
}