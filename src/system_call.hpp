#pragma once

#include <sys/types.h>
#include <system_error>

template <typename T> class system_call_value {
public:
  explicit system_call_value(T value) : value_(value) {
    if (value_ == -1) {
      errno_ = errno;
    }
  }

  system_call_value(const system_call_value &) = delete;
  system_call_value(system_call_value &&) = delete;
  system_call_value &operator=(const system_call_value &) = delete;
  system_call_value &operator=(system_call_value &&) = delete;

  ~system_call_value() = default;

  T value() const {
    if (errno_ != 0) {
      throw std::system_error(errno_, std::system_category());
    }
    return value_;
  }

  operator T() const { return value(); }

  bool is_error() const { return errno_ != 0; }

  T execption(const char* what) const {
    if (errno_ != 0) {
      throw std::system_error(errno_, std::system_category(), what);
    }

    return value_;
  }


private:
  T value_;
  int errno_ = 0;
};

template <typename T> system_call_value<T> system_call(T syscall) {
  return system_call_value<T>(syscall);
}
