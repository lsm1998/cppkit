#include "cppkit/http/http_client.hpp"
#include "cppkit/http/url.hpp"
#include <iostream>

int main()
{
    cppkit::http::HttpClient client{};
    const auto resp = client.Get("http://www.baidu.com/");
    std::cout << "statusCode=" << resp.getStatusCode() << std::endl;
    auto body = resp.getBody();
    std::cout << std::string(body.begin(), body.end()) << std::endl;
    return 0;
}
