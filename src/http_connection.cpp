#include "http_connection.hpp"

namespace co_io {

TaskNoSuspend<void> HttpConnection::handle() {
  ByteBuffer buf{1024};
  while (!stop) {
    std::cerr << "handle waiting for data" << std::endl;
    auto ret = co_await conn_.async_read(buf);
    std::cerr << ret.what() << std::endl;
    if (ret.is_error()) {
      break;
    }
    auto size = ret.value();
    std::cerr << "size = " << size << std::endl;
    if (size == 0 ||
        parser_.parse(buf.span(0, static_cast<size_t>(size))).is_error()) {
      break;
    }
  }
  std::cerr << "handle finished" << std::endl;
}

TaskNoSuspend<void> HttpConnection::handle_request(HttpRequest req) {
  std::string response = "HTTP/1.1 200 OK\r\n";
  response += "Content-Length: " + std::to_string(req.body.size()) + "\r\n";
  response += "\r\n";
  response += req.body;
  std::cerr << response << std::endl;
  auto ret = co_await conn_.async_write(response);
  std::cerr << "async_write " << ret.what() << std::endl;
  if (ret.is_error() || !req.keep_alive()) {
    stop = true;
  }
  co_return;
}

} // namespace co_io
