#pragma once

#include <cstring>
#include <sys/types.h>
#include <system_error>
#include <variant>

namespace co_io {

template <typename T> class Execpted {
  public:
    explicit Execpted(T value) {
        if (value < 0) {
            value_ = std::error_code(errno, std::system_category());
        } else {
            value_ = value;
        }
    }

    explicit Execpted(std::error_code err) : value_(err) {}

    Execpted() = default;
    ~Execpted() = default;

    T value() const {
        if (std::holds_alternative<std::error_code>(value_)) {
            throw std::system_error(std::get<std::error_code>(value_));
        }
        return std::get<T>(value_);
    }

    operator T() const { return value(); }

    bool is_error() const noexcept { return std::holds_alternative<std::error_code>(value_); }
    bool is_errno(int err) const noexcept {
        if (std::holds_alternative<std::error_code>(value_)) {
            return std::get<std::error_code>(value_) ==
                   std::error_code(err, std::system_category());
        }
        return false;
    }

    T execption(const char *what) const {
        if (std::holds_alternative<std::error_code>(value_)) {
            throw std::system_error(std::get<std::error_code>(value_), what);
        }
        return std::get<T>(value_);
    }

    bool is_nonblocking_error() const noexcept {
        return is_errno(EAGAIN) || is_errno(EINTR) || is_errno(EINPROGRESS) ||
               is_errno(EWOULDBLOCK);
    }

    std::string what() const noexcept {
        if (std::holds_alternative<std::error_code>(value_)) {
            return std::get<std::error_code>(value_).message();
        }
        return std::string("no error");
    }

  private:
    std::variant<T, std::error_code> value_;
};

template <typename T> Execpted<T> system_call(T syscall) { return Execpted<T>(syscall); }

} // namespace co_io
