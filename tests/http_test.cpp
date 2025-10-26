#include "cppkit/http/http_client.hpp"
#include <iostream>

int main() {
  cppkit::http::HttpClient client{};
  auto resp = client.Get("http://www.google.com/");
  std::cout << "statusCode=" << resp.getStatusCode() << std::endl;
  auto body = resp.getBody();
  std::cout << std::string(body.begin(), body.end()) << std::endl;
  return 0;
}