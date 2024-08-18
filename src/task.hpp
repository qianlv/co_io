#pragma once
#include <coroutine>
#include <exception>
#include <iostream>

namespace co_io {

template <typename T> struct Promise;
template <> struct Promise<void>;

template <typename Ret = void> class TaskNoSuspend {
public:
  using promise_type = Promise<Ret>;
  TaskNoSuspend(std::coroutine_handle<promise_type> handle)
      : m_handle(handle) {}
  TaskNoSuspend() = default;
  TaskNoSuspend(TaskNoSuspend &&other) = default;
  TaskNoSuspend &operator=(TaskNoSuspend &&other) = default;
  ~TaskNoSuspend() {}

  TaskNoSuspend(const TaskNoSuspend &) = delete;
  TaskNoSuspend &operator=(const TaskNoSuspend &) = delete;

  struct Awaiter {
    std::coroutine_handle<promise_type> callee;
    bool await_ready() { return false; }
    void await_suspend(std::coroutine_handle<> caller) {
      callee.promise().previous_handle_ = caller;
    }
    Ret await_resume() { return callee.promise().result(); }
  };

  Awaiter operator co_await() { return Awaiter{m_handle}; }

  std::coroutine_handle<promise_type> m_handle;
};

template <typename T> struct Promise {
  std::exception_ptr exception_;
  T return_value_;
  std::coroutine_handle<> previous_handle_ = nullptr;
  using handle_type = std::coroutine_handle<Promise>;
  TaskNoSuspend<T> get_return_object() {
    return handle_type::from_promise(*this);
  }
  std::suspend_never initial_suspend() { return {}; }
  std::suspend_never final_suspend() noexcept { return {}; }
  auto return_value(T value) noexcept {
    // std::cerr << "this " << handle_type::from_promise(*this).address()
    //           << std::endl;
    // std::cerr << "return value: " << std::endl;
    return_value_ = std::move(value);
    // std::cerr << "previous_handle_ " << previous_handle_.address() <<
    // std::endl;
    if (previous_handle_) {
      previous_handle_.resume();
    }
  }
  void unhandled_exception() {
    std::cerr << "unhandled exception in coroutine\n";
    exception_ = std::current_exception();
    if (previous_handle_) {
      previous_handle_.resume();
    }
  }

  T result() {
    if (exception_) {
      std::rethrow_exception(exception_);
    }
    return return_value_;
  }
};

template <> struct Promise<void> {
  std::exception_ptr exception_;
  std::coroutine_handle<> previous_handle_ = nullptr;
  using handle_type = std::coroutine_handle<Promise<void>>;
  TaskNoSuspend<void> get_return_object() {
    return handle_type::from_promise(*this);
  }

  std::suspend_never initial_suspend() { return {}; }
  std::suspend_never final_suspend() noexcept { return {}; }

  void return_void() noexcept {
    if (previous_handle_) {
      previous_handle_.resume();
    }
  }

  void unhandled_exception() {
    std::cerr << "unhandled exception in coroutine void\n";
    exception_ = std::current_exception();
    if (previous_handle_) {
      previous_handle_.resume();
    }
  }

  void result() {
    if (exception_) {
      std::rethrow_exception(exception_);
    }
  }
};

} // namespace co_io
