#include "http/http_parser.hpp"
#include <assert.h>
#include <iostream>
#include <ostream>

using namespace co_io;

Task<void> on_request(HttpRequest req) {
    std::cerr << http_method(req.method) << " " << req.url << " " << http_version(req.version)
              << std::endl;
    if (!req.headers.empty()) {
        std::cerr << "headers:\n";
        for (auto &[k, v] : req.headers) {
            std::cerr << k << ": " << v << std::endl;
        }
    }
    if (!req.body.empty()) {
        std::cerr << req.body << std::endl;
    }
    std::cerr << std::endl;

    co_return;
}

int main() {
    HttpPraser parser(on_request);
    std::string request = "POST /index.html HTTP/1.1\r\ncontent-length: "
                          "4\r\n\r\n1234";
    request += "POST /index.html HTTP/1.1\r\n"
               "\r\n";
    request += "POST /index.html HTTP/1.1\r\n"
               "TEST: 3\r\n"
               "\r\n";
    parser.parse(request);
    parser.parse("POST /index.html?key=value HTTP/1.1\r\n\r\n");
    return 0;
}
