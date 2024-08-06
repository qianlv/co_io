#pragma once

#include <coroutine>
#include <spdlog/spdlog.h>

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

template <typename T> class Task {
public:
  struct Promise;
  using promise_type = Promise;
  using handle_type = std::coroutine_handle<promise_type>;

  struct Promise {
    std::exception_ptr exception_;
    T return_value_;
    std::coroutine_handle<> previous_handle_;

    Task get_return_object() { return {handle_type::from_promise(*this)}; }

    std::suspend_never initial_suspend() { return {}; }

    PreviousAwaiter final_suspend() noexcept { return {previous_handle_}; }

    void return_value(T value) noexcept { return_value_ = std::move(value); }

    void unhandled_exception() {
      exception_ = std::current_exception();
      std::rethrow_exception(exception_);
    }
  };

  struct Awaiter {
    handle_type handle_;

    bool await_ready() { return false; }

    void await_suspend(std::coroutine_handle<> handle) {
      handle_.promise().previous_handle_ = handle;
    }

    T await_resume() { return handle_.promise().return_value_; }
  };

  Awaiter operator co_await() { return {handle_}; }

  T get_return_value() { return handle_.promise().return_value_; }

  handle_type get_handle() { return handle_; }

  Task(Task&&) = default;
  Task& operator=(Task&&) = default;

  ~Task() { 
    if (handle_) {
      handle_.destroy();
    }
  }

private:
  handle_type handle_;

  Task(const handle_type &handle) : handle_(handle) {}
  Task(const Task &) = delete;
  Task &operator=(const Task &) = delete;
};

class TaskVoid {
public:
  struct Promise;
  using promise_type = Promise;
  using handle_type = std::coroutine_handle<promise_type>;

  struct Promise {
    std::exception_ptr exception_;
    std::coroutine_handle<> previous_handle_;

    TaskVoid get_return_object() { return {}; }

    std::suspend_never initial_suspend() { return {}; }
    std::suspend_never final_suspend() noexcept { return {}; }

    void unhandled_exception() {
      spdlog::debug("taskvoid unhandled exception");
      exception_ = std::current_exception();
      std::rethrow_exception(exception_);
    }
  };
};
