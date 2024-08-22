#pragma once

#include "adaptive_radix_tree.hpp"
#include "http_endpoint.hpp"
#include "http_util.hpp"
#include <string>

namespace co_io {

class HttpRouter {
public:
  bool route(const std::string &url, HttpMethod method,
             HttpReponseCallback callback, bool use_regex = false) {
    std::string key = url + "_" + std::string(http_method(method));
    HttpEndpoint end_point{std::move(callback), url, method, use_regex};
    if (!end_point.ok()) {
      return false;
    }

    if (!use_regex) {
      match_routes_.insert(key, std::move(end_point));
    } else {
      regex_routes_.insert(key, std::move(end_point));
    }
    return true;
  }

  HttpResponse handle(HttpRequest req) {
    std::string key = req.url + "_" + std::string(http_method(req.method));
    if (auto end_point = match_routes_.search(key); end_point) {
      if ((*end_point).match(req)) {
        return (*end_point)(std::move(req));
      }
    }
    for (auto &[_, end_point] : regex_routes_) {
      if (end_point.match(req)) {
        return end_point(std::move(req));
      }
    }
    return HttpResponse{.status = 404, .body = "<h1>404 Not Found</h1>"}; // 404
  }

private:
  AdaptiveRadixTree<HttpEndpoint> match_routes_;
  AdaptiveRadixTree<HttpEndpoint> regex_routes_;
};

} // namespace co_io
