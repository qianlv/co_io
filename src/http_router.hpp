#pragma once

#include "adaptive_radix_tree.hpp"
#include "http_endpoint.hpp"
#include "http_util.hpp"
#include <functional>
#include <string>

namespace co_io {

class HttpRouter {
public:
  void route(const std::string &url, enum HttpMethod method,
             HttpReponseCallback callback) {
    std::string key = url + "_" + std::string(http_method(method));
    routes_.insert(key, HttpEndpoint{std::move(callback), url, method});
  }

  HttpResponse handle(HttpRequest req) {
    HttpEndpoint end_point;
    std::string key = req.url + "_" + std::string(http_method(req.method));
    if (routes_.search(key, end_point)) {
      if (end_point.match(req)) {
        return end_point(std::move(req));
      }
    }
    return HttpResponse{404}; // 404
  }

private:
  AdaptiveRadixTree<HttpEndpoint> routes_;
};

} // namespace co_io
