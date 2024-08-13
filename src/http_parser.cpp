#include "http_parser.hpp"
#include <functional>
#include <iostream>
#include <ostream>
#include <stdexcept>

namespace co_io {
namespace {} // namespace

HttpPraser::HttpPraser(CallbackRequest on_request_complete)
    : on_request_complete_(std::move(on_request_complete)) {
  llhttp_settings_init(&settings_);
  llhttp_init(&parser_, HTTP_REQUEST, &settings_);
  parser_.data = this;

  // settings_.on_message_complete = HttpPraser::on_headers_complete;
  settings_.on_headers_complete = HttpPraser::on_message_complete;
  settings_.on_reset = HttpPraser::on_reset;
  // settings_.on_chunk_header = HttpPraser::on_chunk_header;
  // settings_.on_chunk_header = HttpPraser::on_chunk_header;

  settings_.on_url = HttpPraser::on_url;
  settings_.on_method = HttpPraser::on_method;
  settings_.on_version = HttpPraser::on_version;
  settings_.on_status = HttpPraser::on_status;
  settings_.on_header_field = HttpPraser::on_header_field;
  settings_.on_header_value = HttpPraser::on_header_value;
  settings_.on_body = HttpPraser::on_body;
}

HttpPraser::~HttpPraser() { llhttp_finish(&parser_); }

detail::Execpted<size_t> HttpPraser::parse(std::string_view data) {
  llhttp_errno_t error = llhttp_execute(&parser_, data.data(), data.size());
  switch (error) {
  case HPE_OK:
    return detail::Execpted{data.size()};
  // case HPE_PAUSED:
  // case HPE_PAUSED_UPGRADE:
  // case HPE_PAUSED_H2_UPGRADE:
  //   return detail::Execpted(
  //       static_cast<size_t>(parser_.error_pos - data.data()));
  default:
    return detail::Execpted<size_t>(
        std::error_code{error, http_parser_category()});
  }
}

int HttpPraser::on_message_complete(llhttp_t *parser) {
  std::cerr << "on_message_complete " << parser->content_length << std::endl;
  HttpPraser *p = static_cast<HttpPraser *>(parser->data);
  if (parser->content_length == 0) {
    p->on_request_complete_(std::move(p->req));
  } else {
    p->content_length = parser->content_length;
  }
  return 0;
}

// int HttpPraser::on_headers_complete(llhttp_t *parser) {
//   HttpPraser *p = static_cast<HttpPraser *>(parser->data);
//   return 0;
// }

int HttpPraser::on_url(llhttp_t *parser, const char *at, size_t length) {
  HttpPraser *p = static_cast<HttpPraser *>(parser->data);
  p->req.url = std::string(at, length);
  return 0;
}

int HttpPraser::on_method(llhttp_t *parser, const char *at, size_t length) {
  HttpPraser *p = static_cast<HttpPraser *>(parser->data);
  p->req.method = std::string(at, length);
  return 0;
}

int HttpPraser::on_version(llhttp_t *parser, const char *at, size_t length) {
  HttpPraser *p = static_cast<HttpPraser *>(parser->data);
  p->req.version = std::string(at, length);
  return 0;
}

int HttpPraser::on_status(llhttp_t *, const char *, size_t) {
  // HttpPraser *p = static_cast<HttpPraser *>(parser->data);
  // p->req.status = std::string(at, length);
  return 0;
}

int HttpPraser::on_header_field(llhttp_t *parser, const char *at,
                                size_t length) {
  HttpPraser *p = static_cast<HttpPraser *>(parser->data);
  p->last_header_field = std::string(at, length);
  return 0;
}

int HttpPraser::on_header_value(llhttp_t *parser, const char *at,
                                size_t length) {
  HttpPraser *p = static_cast<HttpPraser *>(parser->data);
  std::cerr << "on_header_value\n";
  if (p->last_header_field.empty()) {
    return -1;
  }
  p->req.headers.insert_or_assign(std::move(p->last_header_field),
                                  std::string(at, length));
  return 0;
}

int HttpPraser::on_body(llhttp_t *parser, const char *at, size_t length) {
  std::cerr << "on body " << std::string(at, length) << std::endl;
  HttpPraser *p = static_cast<HttpPraser *>(parser->data);
  p->req.body.append(at, length);
  std::cerr << p->req.body.length() << " " << p->content_length << std::endl;
  if (p->req.body.size() == p->content_length) {
    p->on_request_complete_(std::move(p->req));
  }
  return 0;
}

int HttpPraser::on_reset(llhttp_t *parser) {
  std::cerr << "on reset\n";
  HttpPraser *p = static_cast<HttpPraser *>(parser->data);
  p->last_header_field.clear();
  p->req.clear();
  p->content_length = 0;
  return 0;
}

// int HttpPraser::on_chunk_header(llhttp_t *parser) {
//   std::cerr << "on_chunk_header\n";
//   HttpPraser *p = static_cast<HttpPraser *>(parser->data);
//   std::cerr << parser->content_length << std::endl;
//   p->content_length = parser->content_length;
//   return 0;
// }
//
// int HttpPraser::on_chunk_complete(llhttp_t *parser) {
//   std::cerr << "on_chunk_complete\n";
//   HttpPraser *p = static_cast<HttpPraser *>(parser->data);
//   (void)p;
//   return 0;
// }

} // namespace co_io
