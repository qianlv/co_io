#pragma once

#include "task.hpp"
#include <span>

namespace co_io {

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

  void await_resume() const noexcept {}

  std::span<const Task<void>> tasks_;
};

inline Task<void> when_any_helper(const auto& task) { co_await task; }

template <Awaitable... Awaiters> inline Task<void> when_any(Awaiters&&... tasks) {
  auto self = co_await Self{};
  Task<void> new_tasks[]{when_any_helper(tasks)...};
  for (auto &task : new_tasks) {
    task.handle().promise().previous_handle_ = self;
  }

  co_await WhenAnyAwaiter{new_tasks};
}

} // namespace co_io
