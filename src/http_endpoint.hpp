#pragma once

#include "http_util.hpp"
#include <string>
#include "re2/re2.h"

namespace co_io {

class HttpEndpoint {
public:
  HttpEndpoint() = default;
  HttpEndpoint(HttpReponseCallback callback, std::string url, HttpMethod method,
               bool use_regex = false)
      : callback_(std::move(callback)), url_(std::move(url)), method_(method),
        use_regex_{use_regex} {}

  bool match(const HttpRequest &req);

  HttpResponse operator()(HttpRequest req) { return callback_(std::move(req)); }

private:
  HttpReponseCallback callback_;
  std::string url_;
  enum HttpMethod method_;
  bool use_regex_;
  // re2::RE2 pattern_;
};

} // namespace co_io
