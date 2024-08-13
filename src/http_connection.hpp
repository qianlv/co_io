#pragma once

#include "async_file.hpp"
#include "byte_buffer.hpp"
#include "http_parser.hpp"
#include "task.hpp"

#include <functional>

namespace co_io {

class HttpConnection {
public:
  HttpConnection(AsyncFile conn)
      : conn_(std::move(conn)), buffer_(1024),
        parser_(std::bind(&HttpConnection::handle_request, this,
                          std::placeholders::_1)) {
  }

  TaskNoSuspend<void> handle();

private:
  AsyncFile conn_;
  ByteBuffer buffer_;
  HttpPraser parser_;
  bool stop = {false};

  TaskNoSuspend<void> handle_request(HttpRequest req);
};

} // namespace co_io
