#pragma once

#include "adaptive_radix_tree.hpp"
#include "http_util.hpp"
#include <functional>
#include <string>
#include <unordered_map>

namespace co_io {

class HttpRouter {
public:
  using HttpReponseCallback = std::function<HttpResponse(HttpRequest)>;

  void route(const std::string &url, HttpReponseCallback callback) {
    routes_.insert(url, std::move(callback));
  }

  HttpResponse handle(HttpRequest req) {
    HttpReponseCallback callback;
    if (routes_.search(req.url, callback)) {
      return callback(req);
    }
    return HttpResponse{404}; // 404
  }

private:
  AdaptiveRadixTree<HttpReponseCallback> routes_;
};

} // namespace co_io
