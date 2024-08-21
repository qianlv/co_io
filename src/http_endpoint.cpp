#include "http_endpoint.hpp"

namespace co_io {

bool HttpEndpoint::match(const HttpRequest &req) {
  if (req.method != method_) {
    return false;
  }

  // TODO
  if (use_regex_) {
    return true;
  }

  return req.url == url_;
}

} // namespace co_io
