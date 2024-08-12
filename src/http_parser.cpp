#include "http_parser.hpp"
#include <functional>

namespace co_io {
namespace {
template <typename T> struct Callback;

template <typename Ret, typename... Params> struct Callback<Ret(Params...)> {
  template <typename... Args> static Ret callback(Args... args) {
    return func(args...);
  }
  static std::function<Ret(Params...)> func;
};

// Initialize the static member.
template <typename Ret, typename... Params>
std::function<Ret(Params...)> Callback<Ret(Params...)>::func;

} // namespace

HttpPraser::HttpPraser() {
  llhttp_settings_init(&settings_);
  llhttp_init(&parser_, HTTP_REQUEST, &settings_);

#define SETUP_CB(fname)                                                        \
  do {                                                                         \
    Callback<int(llhttp_t *)>::func =                                          \
        std::bind(&HttpPraser::fname, this, std::placeholders::_1);            \
    settings_.fname = static_cast<decltype(settings_.fname)>(                  \
        Callback<int(llhttp_t *)>::callback);                                  \
  } while (1)

#define SETUP_DATA_CB(fname)                                                   \
  do {                                                                         \
    Callback<int(llhttp_t *, const char *, size_t)>::func =                    \
        std::bind(&HttpPraser::fname, this, std::placeholders::_1,             \
                  std::placeholders::_2, std::placeholders::_3);               \
    settings_.fname = static_cast<decltype(settings_.fname)>(                  \
        Callback<int(llhttp_t *, const char *, size_t)>::callback);            \
  } while (1)

  SETUP_CB(on_message_complete);
  SETUP_CB(on_headers_complete);
  SETUP_DATA_CB(on_url);
  SETUP_DATA_CB(on_method);
  SETUP_DATA_CB(on_version);
  SETUP_DATA_CB(on_header_field);
  SETUP_DATA_CB(on_header_value);
  SETUP_DATA_CB(on_body);

#undef SETUP_CB
#undef SETUP_DATA_CB
}

HttpPraser::~HttpPraser() { llhttp_finish(&parser_); }

size_t HttpPraser::parse(std::string_view data) {
  llhttp_errno_t error = llhttp_execute(&parser_, data.data(), data.size());
  switch (error) {
  case HPE_OK:
    return data.size();
  case HPE_PAUSED:
  case HPE_PAUSED_UPGRADE:
  case HPE_PAUSED_H2_UPGRADE:
    return static_cast<size_t>(parser_.error_pos - data.data());
  default:
    throw std::runtime_error(llhttp_get_error_reason(&parser_));
    return 0;
  }
}

bool HttpPraser::is_paused() const { return paused_; }

bool HttpPraser::is_finished() const { return finished_; }

void HttpPraser::reset() {
  llhttp_reset(&parser_);
  finished_ = paused_ = false;
  headers_.clear();
  url_.clear();
  method_.clear();
  body_.clear();
  version_.clear();
  last_header_field_.clear();
}

int HttpPraser::on_message_complete(llhttp_t *) {
  this->finished_ = true;
  return 0;
}

int HttpPraser::on_headers_complete(llhttp_t *) {
  paused_ = true;
  return HPE_PAUSED;
}

int HttpPraser::on_url(llhttp_t *, const char *at, size_t length) {
  url_ = std::string(at, length);
  return 0;
}

int HttpPraser::on_method(llhttp_t *, const char *at, size_t length) {
  method_ = std::string(at, length);
  return 0;
}

int HttpPraser::on_version(llhttp_t *, const char *at, size_t length) {
  version_ = std::string(at, length);
  return 0;
}

int HttpPraser::on_header_field(llhttp_t *, const char *at, size_t length) {
  last_header_field_ = std::string(at, length);
  return 0;
}

int HttpPraser::on_header_value(llhttp_t *, const char *at, size_t length) {
  if (last_header_field_.empty()) {
    return -1;
  }
  headers_.insert_or_assign(std::move(last_header_field_),
                            std::string(at, length));
  return 0;
}

int HttpPraser::on_body(llhttp_t *, const char *at, size_t length) {
  body_ = std::string(at, length);
  return 0;
}

} // namespace co_io
