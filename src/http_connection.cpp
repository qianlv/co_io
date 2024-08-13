#include "http_connection.hpp"

namespace co_io {

TaskNoSuspend<void> HttpConnection::handle() {
  ByteBuffer buf{1024};
  while (!stop) {
    auto ret = co_await conn_.async_read(buf);
    if (ret.is_error()) {
      break;
    }
    auto size = ret.value();
    if (size == 0 ||
        parser_.parse(buf.span(0, static_cast<size_t>(size))).is_error()) {
      break;
    }
  }
}

TaskNoSuspend<void> HttpConnection::handle_request(HttpRequest req) {
  std::string response = "HTTP/1.1 200 OK\r\n";
  std::string body =
      req.body.empty() ? "你好，你的请求正文为空哦" : std::move(req.body);
  response.append("Content-Length: ");
  response.append(std::to_string(body.size()));
  response.append("\r\n\r\n");
  response.append(body);
  auto ret = co_await conn_.async_write(response);
  if (ret.is_error() || !req.keep_alive()) {
    stop = true;
  }
  co_return;
}

} // namespace co_io
