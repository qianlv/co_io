#pragma once

#include "coroutine/task.hpp"
#include "http/http_util.hpp"
#include "llhttp.h"
#include "utils/system_call.hpp"

#include <functional>
#include <string>
#include <system_error>

namespace co_io {

inline std::error_category const &http_parser_category() {
    static struct : std::error_category {
        char const *name() const noexcept override { return "llhttp"; }
        std::string message(int err) const override {
            return llhttp_errno_name(static_cast<llhttp_errno_t>(err));
        }
    } instance;
    return instance;
};

class HttpPraser {
  public:
    using CallbackRequest = std::function<Task<void>(HttpRequest)>;
    HttpPraser(CallbackRequest on_request_complete);
    ~HttpPraser();

    Execpted<size_t> parse(std::string_view data);

  private:
    CallbackRequest on_request_complete_;
    llhttp_t parser_;
    llhttp_settings_t settings_;
    std::string last_header_field = {};
    uint64_t content_length = {};
    HttpRequest req;

    static int on_url(llhttp_t *parser, const char *at, size_t length);
    static int on_method(llhttp_t *parser, const char *at, size_t length);
    static int on_version(llhttp_t *parser, const char *at, size_t length);
    static int on_status(llhttp_t *parser, const char *at, size_t length);
    static int on_header_field(llhttp_t *parser, const char *at, size_t length);
    static int on_header_value(llhttp_t *parser, const char *at, size_t length);
    static int on_body(llhttp_t *parser, const char *at, size_t length);

    // static int on_headers_complete(llhttp_t *);
    static int on_message_complete(llhttp_t *);
    static int on_reset(llhttp_t *parser);
    // static int on_chunk_header(llhttp_t *parser);
    // static int on_chunk_complete(llhttp_t *parser);
};
} // namespace co_io
