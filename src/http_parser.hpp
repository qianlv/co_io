#pragma once

#include "byte_buffer.hpp"
#include "llhttp.h"
#include <unordered_map>

namespace co_io {

class HttpPraser {
public:
  HttpPraser();
  ~HttpPraser();
  void reset();
  bool is_finished() const;
  bool is_paused() const;

  size_t parse(std::string_view data);
private:
  llhttp_t parser_;
  llhttp_settings_t settings_;
  bool finished_ = {false};
  bool paused_ = {false};

  std::unordered_map<std::string, std::string> headers_;
  std::string url_;
  std::string method_;
  std::string body_;
  std::string version_;
  std::string last_header_field_;

  int on_url(llhttp_t* parser, const char* at, size_t length);
  int on_method(llhttp_t* parser, const char* at, size_t length);
  int on_version(llhttp_t* parser, const char* at, size_t length);
  int on_header_field(llhttp_t* parser, const char* at, size_t length);
  int on_header_value(llhttp_t* parser, const char* at, size_t length);
  int on_body(llhttp_t* parser, const char* at, size_t length);

  int on_headers_complete(llhttp_t*);
  int on_message_complete(llhttp_t *);
};
} // namespace co_io
