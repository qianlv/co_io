#pragma once

#include "coroutine/task.hpp"
#include "http/http_parser.hpp"
#include "http/http_router.hpp"
#include "io/async_file.hpp"
#include "utils/byte_buffer.hpp"

#include <functional>

namespace co_io {

class HttpConnection {
public:
  HttpConnection(AsyncFile conn, HttpRouter &router)
      : conn_(std::move(conn)), buffer_(1024), router_(router),
        parser_(std::bind(&HttpConnection::handle_request, this,
                          std::placeholders::_1)) {}

  Task<void> handle();

private:
  AsyncFile conn_;
  ByteBuffer buffer_;
  HttpRouter &router_;
  HttpPraser parser_;
  bool stop = {false};

  Task<void> handle_request(HttpRequest req);
};

} // namespace co_io
