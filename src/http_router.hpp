#pragma once

#include "http_util.hpp"
#include <functional>
#include <string>
#include <unordered_map>

namespace co_io {

class HttpRouter {
public:
  using HttpReponseCallback = std::function<HttpResponse(HttpRequest)>;

  void route(const std::string &url, HttpReponseCallback callback) {
    routes_.insert_or_assign(std::string(url), std::move(callback));
  }

  HttpResponse handle(HttpRequest req) {
    auto it = routes_.find(req.url);
    if (it != routes_.end()) {
      return it->second(std::move(req));
    }
    return HttpResponse{404}; // 404
  }

private:
  std::unordered_map<std::string, HttpReponseCallback> routes_;
};

} // namespace co_io
