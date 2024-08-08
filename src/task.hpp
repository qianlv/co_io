#pragma once

#include <coroutine>
#include <exception>

namespace co_io {

template <typename Ret> class TaskNoSuspend {
private:
  template <typename T> struct Promise {
    std::exception_ptr exception_;
    T return_value_;
    std::coroutine_handle<> previous_handle_;
    using handle_type = std::coroutine_handle<Promise>;
    TaskNoSuspend<T> get_return_object() { return {}; }
    std::suspend_never initial_suspend() { return {}; }
    std::suspend_never final_suspend() noexcept { return {}; }
    void return_value(T value) noexcept { return_value_ = std::move(value); }
    void unhandled_exception() {}
  };

  template <> struct Promise<void> {
    TaskNoSuspend<void> get_return_object() { return {}; }
    std::suspend_never initial_suspend() { return {}; }
    std::suspend_never final_suspend() noexcept { return {}; }
    void return_void() noexcept {}
    void unhandled_exception() {}
  };

public:
  using promise_type = Promise<Ret>;
  TaskNoSuspend() = default;
  TaskNoSuspend(TaskNoSuspend &&other) = default;
  TaskNoSuspend &operator=(TaskNoSuspend &&other) = default;
  ~TaskNoSuspend() {}

  TaskNoSuspend(const TaskNoSuspend &) = delete;
  TaskNoSuspend &operator=(const TaskNoSuspend &) = delete;
};

} // namespace co_io
