#pragma once

#include <cstring>
#include <sys/types.h>
#include <system_error>

namespace co_io {
namespace detail {

template <typename T> class system_call_value {
public:
  explicit system_call_value(T value) : value_(value) {
    if (value_ == -1) {
      errno_ = errno;
    }
  }

  system_call_value() = default;
  ~system_call_value() = default;

  T value() const {
    if (errno_ != 0) {
      throw std::system_error(errno_, std::system_category());
    }
    return value_;
  }

  T raw_value() const { return value_; }

  operator T() const { return value(); }

  bool is_error() const noexcept { return value_ < 0 && errno_ != 0; }
  bool is_error(int err) const noexcept { return value_ < 0 && errno_ == err; }

  T execption(const char *what) const {
    if (errno_ != 0) {
      throw std::system_error(errno_, std::system_category(), what);
    }

    return value_;
  }

  bool is_nonblocking_error() const noexcept {
    return value_ < 0 &&
           (errno_ == EAGAIN || errno_ == EINTR || errno_ == EINPROGRESS || errno_ == EWOULDBLOCK);
  }

  const char* what() const noexcept { return strerror(errno_); }

private:
  T value_{};
  int errno_ = 0;
};

template <typename T> system_call_value<T> system_call(T syscall) {
  return system_call_value<T>(syscall);
}

} // namespace detail

} // namespace co_io
