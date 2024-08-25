#pragma once

#include "task.hpp"
#include <span>

namespace co_io {

template <typename T>
struct WhenAnyResult {
  T value;
  size_t index;
};

struct WhenAnyAwaiter {
  bool await_ready() const noexcept { return false; }

  std::coroutine_handle<> await_suspend(std::coroutine_handle<> h) const {
    if (tasks_.empty()) {
      return h;
    }
    for (const auto &task : tasks_.subspan(0, tasks_.size() - 1)) {
      task.handle().promise().previous_handle_ = h;
    }
    return tasks_.back().handle();
  }

  void await_resume() const noexcept { }

  std::span<const Task<size_t>> tasks_;
};

inline Task<size_t> when_any_helper(const auto &task, size_t index) {
  co_await task;
  co_return index;
}

template <std::size_t... Is, Awaitable... Awaiters>
inline Task<size_t> when_any_impl(std::index_sequence<Is...>,
                                  Awaiters &&...tasks) {
  auto self = co_await Self{};
  Task<size_t> new_tasks[]{when_any_helper(tasks, Is)...};
  for (auto &task : new_tasks) {
    task.handle().promise().previous_handle_ = self;
  }

  co_return co_await WhenAnyAwaiter{new_tasks};
}

template <Awaitable... Awaiters>
  requires(sizeof...(Awaiters) > 0)
inline Task<size_t> when_any(Awaiters &&...tasks) {
  return when_any_impl(std::make_index_sequence<sizeof...(Awaiters)>{},
                       std::forward<Awaiters>(tasks)...);
}

} // namespace co_io
