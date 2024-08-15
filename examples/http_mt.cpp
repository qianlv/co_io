#include "http_server.hpp"
#include "loop.hpp"

int main() {
  co_io::HttpServer<co_io::EPollLoop> http("localhost", "12345");
  http.with_threads();
  // co_io::HttpServer<co_io::SelectLoop> http("localhost", "12345");
  http.route().route("/", [](co_io::HttpRequest req) -> co_io::HttpResponse {
    (void)req;
    co_io::HttpResponse res{200};
    res.headers["Content-Type"] = "text/plain;charset=utf-8";
    res.headers["Connection"] = "keep-alive";
    res.body = "<h1>Hello World! Hello World! Hello World! Hello World! Hello World! Hello World! </h1>";
    return res;
  });
  try {
    http.start();
  } catch (const std::exception &e) {
    std::cerr << e.what() << std::endl;
  }
  return 0;
}
