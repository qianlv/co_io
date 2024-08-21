#pragma once

#include "http_util.hpp"
#include <string>
namespace co_io {

class HttpEndpoint {
public:
  HttpEndpoint() = default;
  HttpEndpoint(HttpReponseCallback callback, std::string url, HttpMethod method)
      : callback_(std::move(callback)), url_(std::move(url)), method_(method),
        use_regex_{false} {}

  bool match(const HttpRequest &req);

  HttpResponse operator()(HttpRequest req) {
    return callback_(std::move(req));
  }

private:
  HttpReponseCallback callback_;
  std::string url_;
  enum HttpMethod method_;
  bool use_regex_;
};

} // namespace co_io
