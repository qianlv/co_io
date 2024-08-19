#pragma once

#include <iomanip>
#include <iostream>
#include <string>
#include <unordered_map>

#include "byte_buffer.hpp"

namespace co_io {

std::string_view http_status(int status);
std::vector<std::string_view> split(std::string_view s, std::string_view sep,
                                    size_t max_split = 0);

struct UrlCodec {
  static constexpr char URL_RESERVED[] = "!*'();:@&=+$,/?%#[]";
  static constexpr char URL_UNRESERVED[] =
      "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-_.~";
  static constexpr char URL_PCT[] = "0123456789ABCDEF";
  static std::string encode_url(std::string_view str);
  static std::string decode_url(std::string_view str,
                                bool plus_as_space = false);
};

struct HttpRequest {
  std::unordered_map<std::string, std::string> headers;
  std::unordered_map<std::string, std::string> args;
  std::string url;
  std::string method;
  std::string body;
  std::string version;

  bool keep_alive() const {
    auto it = headers.find("Connection");
    if (it != headers.end()) {
      return it->second == "keep-alive";
    }

    if (version == "HTTP/1.0") {
      return false;
    }
    return true;
  }

  void set_url(std::string_view path) {
    auto pos = path.find('?');
    if (pos != std::string::npos) {
      url = UrlCodec::decode_url(path.substr(0, pos));

      auto url_args = split(path.substr(pos + 1), "&");
      for (std::string_view it : url_args) {
        auto pairs = split(it, "=", 2);
        if (pairs.size() == 2) {
          args.emplace(UrlCodec::decode_url(pairs[0]),
                       UrlCodec::decode_url(pairs[1]));
        } else {
          args.emplace(UrlCodec::decode_url(pairs[0]), "");
        }
      }
    } else {
      url = UrlCodec::decode_url(path);
    }
  }

  void clear() {
    headers.clear();
    url.clear();
    method.clear();
    body.clear();
    version.clear();
  }
};

struct HttpResponse {
public:
  HttpResponse(int status = 200) : status(status) {}

  void serialize(ByteBuffer &buf) const {
    buf.append("HTTP/1.1 ");
    buf.append(std::to_string(status));
    buf.append(" ");
    buf.append(co_io::http_status(status));
    buf.append("\r\n");

    buf.append("Content-Length: " + std::to_string(body.size()) + "\r\n");
    for (auto &it : headers) {
      buf.append(it.first + ": " + it.second + "\r\n");
    }
    buf.append("\r\n");
    buf.append(body);
  }

public:
  std::unordered_map<std::string, std::string> headers;
  std::string body;
  int status = 200;
};

} // namespace co_io
