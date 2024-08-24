#pragma once
#include <coroutine>
#include <exception>
#include <iostream>
#include <utility>
#include <variant>

namespace co_io {

// template <typename T> struct TmpPromise;
// template <> struct TmpPromise<void>;
//
// template <typename Ret = void> class TaskNoSuspend {
// public:
//   using promise_type = TmpPromise<Ret>;
//   TaskNoSuspend(std::coroutine_handle<promise_type> handle)
//       : m_handle(handle) {}
//   TaskNoSuspend() = default;
//   TaskNoSuspend(TaskNoSuspend &&other) = default;
//   TaskNoSuspend &operator=(TaskNoSuspend &&other) = default;
//   ~TaskNoSuspend() {}
//
//   TaskNoSuspend(const TaskNoSuspend &) = delete;
//   TaskNoSuspend &operator=(const TaskNoSuspend &) = delete;
//
//   struct Awaiter {
//     std::coroutine_handle<promise_type> callee;
//     bool await_ready() { return false; }
//     void await_suspend(std::coroutine_handle<> caller) {
//       callee.promise().previous_handle_ = caller;
//     }
//     Ret await_resume() { return callee.promise().result(); }
//   };
//
//   Awaiter operator co_await() { return Awaiter{m_handle}; }
//
//   std::coroutine_handle<promise_type> m_handle;
// };
//
// template <typename T> struct TmpPromise {
//   std::exception_ptr exception_;
//   T return_value_;
//   std::coroutine_handle<> previous_handle_ = nullptr;
//   using handle_type = std::coroutine_handle<TmpPromise>;
//   TaskNoSuspend<T> get_return_object() {
//     return handle_type::from_promise(*this);
//   }
//   std::suspend_never initial_suspend() { return {}; }
//   std::suspend_never final_suspend() noexcept { return {}; }
//   auto return_value(T value) noexcept {
//     // std::cerr << "this " << handle_type::from_promise(*this).address()
//     //           << std::endl;
//     // std::cerr << "return value: " << std::endl;
//     return_value_ = std::move(value);
//     // std::cerr << "previous_handle_ " << previous_handle_.address() <<
//     // std::endl;
//     if (previous_handle_) {
//       previous_handle_.resume();
//     }
//   }
//   void unhandled_exception() {
//     std::cerr << "unhandled exception in coroutine\n";
//     exception_ = std::current_exception();
//     if (previous_handle_) {
//       previous_handle_.resume();
//     }
//   }
//
//   T result() {
//     if (exception_) {
//       std::rethrow_exception(exception_);
//     }
//     return return_value_;
//   }
// };
//
// template <> struct TmpPromise<void> {
//   std::exception_ptr exception_;
//   std::coroutine_handle<> previous_handle_ = nullptr;
//   using handle_type = std::coroutine_handle<TmpPromise<void>>;
//   TaskNoSuspend<void> get_return_object() {
//     return handle_type::from_promise(*this);
//   }
//
//   std::suspend_never initial_suspend() { return {}; }
//   std::suspend_never final_suspend() noexcept { return {}; }
//
//   void return_void() noexcept {
//     if (previous_handle_) {
//       previous_handle_.resume();
//     }
//   }
//
//   void unhandled_exception() {
//     std::cerr << "unhandled exception in coroutine void\n";
//     exception_ = std::current_exception();
//     if (previous_handle_) {
//       previous_handle_.resume();
//     }
//   }
//
//   void result() {
//     if (exception_) {
//       std::rethrow_exception(exception_);
//     }
//   }
// };

struct PreviousAwaiter {
  std::coroutine_handle<> previous_handle_;

  bool await_ready() const noexcept { return false; }

  std::coroutine_handle<>
  await_suspend(std::coroutine_handle<>) const noexcept {
    return previous_handle_;
  }

  void await_resume() const noexcept {}
};

template <typename T = void> struct Promise {
  std::coroutine_handle<> previous_handle_{};
  std::variant<T, std::exception_ptr> result_{};

  auto get_return_object() {
    return std::coroutine_handle<Promise>::from_promise(*this);
  }

  std::suspend_always initial_suspend() { return {}; }
  auto final_suspend() noexcept { return PreviousAwaiter{previous_handle_}; }
  auto return_value(T &&value) { result_ = std::move(value); }
  auto return_value(const T &value) noexcept { result_ = value; }
  void unhandled_exception() { result_ = std::current_exception(); }

  T result() {
    auto r = std::get_if<T>(&result_);
    if (r) [[likely]] {
      return std::move(*r);
    }
    std::rethrow_exception(std::get<std::exception_ptr>(result_));
  }

  Promise &operator=(Promise &&) = delete;
};

template <> struct Promise<void> {
  std::coroutine_handle<> previous_handle_{};
  std::exception_ptr result_{};

  auto get_return_object() {
    return std::coroutine_handle<Promise>::from_promise(*this);
  }

  std::suspend_always initial_suspend() { return {}; }
  auto final_suspend() noexcept { return PreviousAwaiter{previous_handle_}; }
  auto return_void() noexcept { result_ = nullptr; }
  void unhandled_exception() { result_ = std::current_exception(); }

  void result() {
    if (result_) {
      std::rethrow_exception(result_);
    }
  }

  Promise &operator=(Promise &&) = delete;
};

template <typename T, typename P = Promise<T>> class Task {
public:
  using promise_type = P;

  Task(std::coroutine_handle<promise_type> handle) noexcept : handle_(handle) {
  }
  Task(Task &&other) noexcept : handle_(other.handle_) {
    other.handle_ = nullptr;
  }
  Task &operator=(Task &&other) noexcept {
    using std::swap;
    swap(handle_, other.handle_);
    return *this;
  }
  Task(const Task &) = delete;
  Task &operator=(const Task &) = delete;

  ~Task() {
    if (handle_) {
      handle_.destroy();
    }
  }

  struct Awaiter {
    std::coroutine_handle<promise_type> handle_;

    bool await_ready() const noexcept { return false; }

    std::coroutine_handle<promise_type>
    await_suspend(std::coroutine_handle<> h) const noexcept {
      handle_.promise().previous_handle_ = h;
      return handle_;
    }

    T await_resume() const { return handle_.promise().result(); }
  };

  Awaiter operator co_await() { return {handle_}; }

  std::coroutine_handle<promise_type> handle() const noexcept {
    return handle_;
  }

  std::coroutine_handle<promise_type> release() noexcept {
    return std::exchange(handle_, nullptr);
  }


private:
  std::coroutine_handle<promise_type> handle_;
};

struct AutoDestoryPromise {
  struct AutoDestoryAwaiter {
    bool await_ready() const noexcept { return false; }
    void await_suspend(std::coroutine_handle<> h) const noexcept {
      h.destroy();
    }
    void await_resume() const noexcept {}
  };

  auto get_return_object() {
    return std::coroutine_handle<AutoDestoryPromise>::from_promise(*this);
  }

  std::suspend_always initial_suspend() { return {}; }
  auto final_suspend() noexcept { return AutoDestoryAwaiter{}; }
  auto return_void() noexcept {}
  void unhandled_exception() { std::terminate(); }

  void result() {}

  AutoDestoryPromise &operator=(AutoDestoryPromise &&) = delete;
};

template <typename T>
Task<void, AutoDestoryPromise> auto_destory(Task<T> task) {
  (void)co_await task;
}

template <typename T>
auto run_task(Task<T> task) {
  auto wrapper = auto_destory(std::move(task));
  auto handle = wrapper.release();
  handle.resume();
}

} // namespace co_io
