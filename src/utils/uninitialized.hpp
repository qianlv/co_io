#pragma once

#include <memory>
#include <type_traits>
#include <utility>
template <typename T> class Uninitialized {
public:
  Uninitialized() noexcept {}

  Uninitialized(Uninitialized &&other) = delete;

  ~Uninitialized() {}

  template <typename U>
    requires(std::is_constructible_v<T, U>)
  Uninitialized(U &&value) noexcept(std::is_nothrow_constructible_v<T, U>) {
    std::construct_at(&this->value, std::forward<U>(value));
  }

  template <typename U>
    requires(std::is_constructible_v<T, U>)
  Uninitialized &
  operator=(U &&value) noexcept(std::is_nothrow_constructible_v<T, U>) {
    std::construct_at(&this->value, std::forward<U>(value));
    return *this;
  }

  template <typename... Args>
    requires(std::constructible_from<T, Args...>)
  auto emplace(Args &&...args) noexcept(
      std::is_nothrow_constructible_v<T, Args...>) {
    std::construct_at(&this->value, std::forward<Args>(args)...);
  }

  T &get() noexcept { return this->value; }
  const T &get() const noexcept { return this->value; }

  T *operator->() noexcept { return &this->value; }
  const T *operator->() const noexcept { return &this->value; }

  T move() {
    T tmp(std::move(this->value));
    this->value.~T();
    return tmp;
  }

private:
  union {
    T value;
  };
};
