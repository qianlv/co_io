#include "http/http_endpoint.hpp"

namespace co_io {

bool HttpEndpoint::match(const HttpRequest &req) {
    if (req.method != method_) {
        return false;
    }

    if (regex_) {
        return re2::RE2::FullMatch(req.url, *regex_, nullptr);
    }

    return true;
}

} // namespace co_io
