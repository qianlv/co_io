#include "http/http_util.hpp"
#include <algorithm>

namespace co_io {

std::string_view http_status(int status) {
  using namespace std::string_view_literals;
  static const std::unordered_map<int, std::string_view> status_map = {
      {100, "Continue"sv},
      {101, "Switching Protocols"sv},
      {102, "Processing"sv},
      {200, "OK"sv},
      {201, "Created"sv},
      {202, "Accepted"sv},
      {203, "Non-Authoritative Information"sv},
      {204, "No Content"sv},
      {205, "Reset Content"sv},
      {206, "Partial Content"sv},
      {207, "Multi-Status"sv},
      {208, "Already Reported"sv},
      {226, "IM Used"sv},
      {300, "Multiple Choices"sv},
      {301, "Moved Permanently"sv},
      {302, "Found"sv},
      {303, "See Other"sv},
      {304, "Not Modified"sv},
      {305, "Use Proxy"sv},
      {306, "Switch Proxy"sv},
      {307, "Temporary Redirect"sv},
      {308, "Permanent Redirect"sv},
      {400, "Bad Request"sv},
      {401, "Unauthorized"sv},
      {402, "Payment Required"sv},
      {403, "Forbidden"sv},
      {404, "Not Found"sv},
      {405, "Method Not Allowed"sv},
      {406, "Not Acceptable"sv},
      {407, "Proxy Authentication Required"sv},
      {408, "Request Timeout"sv},
      {409, "Conflict"sv},
      {410, "Gone"sv},
      {411, "Length Required"sv},
      {412, "Precondition Failed"sv},
      {413, "Payload Too Large"sv},
      {414, "URI Too Long"sv},
      {415, "Unsupported Media Type"sv},
      {416, "Range Not Satisfiable"sv},
      {417, "Expectation Failed"sv},
      {418, "I'm a teapot"sv},
      {421, "Misdirected Request"sv},
      {422, "Unprocessable Entity"sv},
      {423, "Locked"sv},
      {424, "Failed Dependency"sv},
      {426, "Upgrade Required"sv},
      {428, "Precondition Required"sv},
      {429, "Too Many Requests"sv},
      {431, "Request Header Fields Too Large"sv},
      {451, "Unavailable For Legal Reasons"sv},
      {500, "Internal Server Error"sv},
      {501, "Not Implemented"sv},
      {502, "Bad Gateway"sv},
      {503, "Service Unavailable"sv},
      {504, "Gateway Timeout"sv},
      {505, "HTTP Version Not Supported"sv},
      {506, "Variant Also Negotiates"sv},
      {507, "Insufficient Storage"sv},
      {508, "Loop Detected"sv},
      {510, "Not Extended"sv},
      {511, "Network Authentication Required"sv},
  };
  if (status == 200) {
    return "OK"sv;
  }
  if (auto it = status_map.find(status); it != status_map.end()) {
    return it->second;
  }
  return "Unknown Status"sv;
}

static inline int hex_to_val(char c) {
  if (c >= '0' && c <= '9') {
    return c - '0';
  } else if (c >= 'A' && c <= 'F') {
    return c - 'A' + 10;
  } else if (c >= 'a' && c <= 'f') {
    return c - 'a' + 10;
  }
  return -1;
}

std::string UrlCodec::decode_url(std::string_view str, bool plus_as_space) {
  if (str.size() < 3) {
    return std::string(str);
  }

  std::string decoded;
  size_t i = 0;
  for (i = 0; i + 2 < str.size(); i++) {
    if (str[i] == '%') {
      decoded.push_back(static_cast<char>((hex_to_val(str[i + 1]) << 4) |
                                          hex_to_val(str[i + 2])));
      i += 2;
    } else if (plus_as_space && str[i] == '+') {
      decoded.push_back(' ');
    } else {
      decoded.push_back(str[i]);
    }
  }
  decoded.append(str.substr(i));

  return decoded;
}

std::vector<std::string_view> split(std::string_view s, std::string_view sep,
                                    size_t max_split) {
  std::vector<std::string_view> ret;
  for (size_t i = 0;
       i < s.size() && (max_split == 0 || ret.size() < max_split);) {
    auto j = s.find(sep, i);
    if (j == std::string_view::npos) {
      ret.push_back(s.substr(i));
      break;
    }
    ret.push_back(s.substr(i, j - i));
    i = j + sep.size();
  }
  return ret;
}

std::string_view http_method(HttpMethod method) {
  using namespace std::string_view_literals;
  switch (method) {
  case HttpMethod::GET:
    return "GET"sv;
  case HttpMethod::POST:
    return "POST"sv;
  case HttpMethod::PUT:
    return "PUT"sv;
  case HttpMethod::DELETE:
    return "DELETE"sv;
  case HttpMethod::HEAD:
    return "HEAD"sv;
  case HttpMethod::PATCH:
    return "PATCH"sv;
  case HttpMethod::OPTIONS:
    return "OPTIONS"sv;
  case HttpMethod::TRACE:
    return "TRACE"sv;
  case HttpMethod::CONNECT:
    return "CONNECT"sv;
  default:
    return "Unknown Method"sv;
  }
}

enum HttpMethod http_method(std::string_view m) {
  std::string method(m);
  std::transform(method.begin(), method.end(), method.begin(),
                 [](unsigned char c) { return std::toupper(c); });
  if (method == "GET") {
    return HttpMethod::GET;
  } else if (method == "POST") {
    return HttpMethod::POST;
  } else if (method == "PUT") {
    return HttpMethod::PUT;
  } else if (method == "DELETE") {
    return HttpMethod::DELETE;
  } else if (method == "HEAD") {
    return HttpMethod::HEAD;
  } else if (method == "PATCH") {
    return HttpMethod::PATCH;
  } else if (method == "OPTIONS") {
    return HttpMethod::OPTIONS;
  } else if (method == "TRACE") {
    return HttpMethod::TRACE;
  } else if (method == "CONNECT") {
    return HttpMethod::CONNECT;
  }
  throw std::runtime_error("Unknown Method");
}

enum HttpVersion http_version(std::string_view version) {
  if (version == "1.0") {
    return HttpVersion::HTTP_1_0;
  } else if (version == "1.1") {
    return HttpVersion::HTTP_1_1;
  }
  throw std::runtime_error("Unknown Version");
}

std::string_view http_version(HttpVersion version) {
  using namespace std::string_view_literals;
  switch (version) {
  case HttpVersion::HTTP_1_0:
    return "HTTP/1.0"sv;
  case HttpVersion::HTTP_1_1:
    return "HTTP/1.1"sv;
  }
  throw std::runtime_error("Unknown Version");
}

} // namespace co_io
