#pragma once

#include <string>
#include <unordered_map>

#include "byte_buffer.hpp"

namespace co_io {

std::string_view http_status(int status);

struct HttpRequest {
  std::unordered_map<std::string, std::string> headers;
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
