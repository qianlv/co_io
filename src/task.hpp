#pragma once

#include <coroutine>
#include <spdlog/spdlog.h>

namespace co_io {
struct PreviousAwaiter {
  std::coroutine_handle<> mPrevious;

  bool await_ready() const noexcept { return false; }

  std::coroutine_handle<>
  await_suspend(std::coroutine_handle<>) const noexcept {
    if (mPrevious)
      return mPrevious;
    else
      return std::noop_coroutine();
  }

  void await_resume() const noexcept {}
};

struct SupsendAwaiter {
  bool await_ready() const noexcept { return false; }
  void await_suspend(std::coroutine_handle<>) const noexcept {}
  void await_resume() const {}
};

template <typename T> class Task;

template <typename T> struct Promise {
  std::exception_ptr exception_;
  T return_value_;
  std::coroutine_handle<> previous_handle_;
  using handle_type = std::coroutine_handle<Promise>;

  handle_type get_return_object() { return handle_type::from_promise(*this); }

  std::suspend_never initial_suspend() { return {}; }

  PreviousAwaiter final_suspend() noexcept { return {previous_handle_}; }

  void return_value(T value) noexcept { return_value_ = std::move(value); }

  void unhandled_exception() { exception_ = std::current_exception(); }

  T result() {
    if (exception_) {
      std::rethrow_exception(exception_);
    }
    return std::move(return_value_);
  }
};

template <> struct Promise<void> {
  std::exception_ptr exception_;
  std::coroutine_handle<> previous_handle_;
  using handle_type = std::coroutine_handle<Promise>;

  handle_type get_return_object() { return handle_type::from_promise(*this); }

  std::suspend_never initial_suspend() { return {}; }

  PreviousAwaiter final_suspend() noexcept { return {previous_handle_}; }

  void return_void() noexcept {}

  void unhandled_exception() { exception_ = std::current_exception(); }

  void result() {
    if (exception_) [[unlikely]] {
      std::rethrow_exception(exception_);
    }
  }
};

template <typename T = void> class Task {
public:
  using promise_type = Promise<T>;
  using handle_type = promise_type::handle_type;

  struct Awaiter {
    handle_type handle_;

    bool await_ready() { return false; }

    auto await_suspend(std::coroutine_handle<> handle) {
      handle_.promise().previous_handle_ = handle;
    }

    T await_resume() { return handle_.promise().result(); }
  };

  Awaiter operator co_await() { return {handle_}; }

  handle_type get_handle() { return handle_; }

  Task(handle_type handle) : handle_(handle) {}
  Task(Task &&other) : handle_(other.handle_) { other.handle_ = nullptr; }
  Task &operator=(Task &&other) {
    std::swap(other.handle_, handle_);
    return *this;
  }

  Task() = default;

  ~Task() {
    if (handle_ && handle_.done()) {
      spdlog::debug("desctructor: {}", handle_.address());
      handle_.destroy();
    }
  }

private:
  handle_type handle_;

  // Task(Task &&other) = delete;
  Task(Task const &) = delete;
  Task &operator=(Task const &) = delete;
};

} // namespace co_io
