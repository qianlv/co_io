#include "http_parser.hpp"
#include <assert.h>
#include <iostream>
#include <ostream>

using namespace co_io;

TaskNoSuspend<void> on_request(HttpRequest req) {
  std::cerr << "\non request\n";
  std::cerr << req.method << " " << req.url << " " << req.version << std::endl;
  std::cerr << "headers:\n";
  for (auto &[k, v] : req.headers) {
    std::cerr << k << ": " << v << std::endl;
  }
  std::cerr << req.body << std::endl;

  co_return;
}

int main() {
  HttpPraser parser(on_request);
  std::string request =
      "POST /index.html HTTP/1.1\r\ncontent-length: "
      "4\r\n\r\n1234";
  request += 
      "POST /index.html HTTP/1.1\r\n"
      "\r\n";
  request += 
      "POST /index.html HTTP/1.1\r\n"
      "Test: 3\r\n"
      "\r\n";
  // std::cerr << request;
  auto n = parser.parse(request);
  std::cerr << "n = " << n.value() << " " << request.length() << std::endl;
  // assert(n.value() == request.size() / 2);
  // assert(parser.is_finished());
  return 0;
}
