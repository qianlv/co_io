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
  auto response = router_.handle(std::move(req));
  ByteBuffer buf;
  response.serialize(buf);
  auto ret = co_await conn_.async_write(buf);
  if (ret.is_error() || !req.keep_alive()) {
    stop = true;
  }
  co_return;
}

} // namespace co_io
