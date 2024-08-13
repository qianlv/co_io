#include "http_server.hpp"
#include "loop.hpp"

int main() {
  co_io::HttpServer<co_io::SelectLoop> http("localhost", "12345");
  http.start();
  return 0;
}
