#pragma once

#include "http_util.hpp"
#include "re2/re2.h"
#include <string>

namespace co_io {

class HttpEndpoint {
public:
  HttpEndpoint(HttpReponseCallback callback, std::string url, HttpMethod method,
               bool use_regex = false)
      : callback_(std::move(callback)), url_(std::move(url)), method_(method) {
    if (use_regex) {
      regex_ = std::make_shared<re2::RE2>(url_);
    }
  }

  bool match(const HttpRequest &req);

  bool ok() const { return regex_ == nullptr || regex_->ok(); }

  HttpResponse operator()(HttpRequest req) { return callback_(std::move(req)); }

private:
  HttpReponseCallback callback_;
  std::string url_;
  enum HttpMethod method_;
  std::shared_ptr<re2::RE2> regex_;
};

} // namespace co_io
