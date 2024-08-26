#pragma once

#include "coroutine/task.hpp"
#include "utils/void_type.hpp"
#include "utils/concepts.hpp"
#include <span>
#include <variant>
#include <vector>

namespace co_io {

template <typename T> struct WhenAnyResult {
  T value;
  size_t index = {size_t(-1)};
};

struct WhenAnyAwaiter {
  bool await_ready() const noexcept { return false; }

  std::coroutine_handle<> await_suspend(std::coroutine_handle<> h) const {
    if (tasks_.empty()) {
      return h;
    }
    for (const auto &task : tasks_.subspan(0, tasks_.size() - 1)) {
      task.handle().resume();
    }
    return tasks_.back().handle();
  }

  void await_resume() const noexcept {}

  std::span<const Task<void>> tasks_;
};

template <size_t index, typename T>
inline Task<void> when_any_helper(const auto &task, WhenAnyResult<T> &result) {
  using return_type = typename std::decay_t<decltype(task)>::return_type;
  if constexpr (std::is_void_v<return_type>) {
    co_await task;
  } else { // multiple types, std::variant
    result.value.template emplace<index>(co_await task);
  }
  result.index = index;
}

template <typename T>
inline Task<void> when_any_helper(const auto &task, WhenAnyResult<T> &result,
                                  size_t index) {
  using return_type = typename std::decay_t<decltype(task)>::return_type;
  if constexpr (std::is_void_v<return_type>) {
    co_await task;
  } else { // only one type for return_type
    result.value = (co_await task);
  }
  result.index = index;
}

template <std::size_t... Is, Awaitable... Awaiters>
  requires(HasReturnType<Awaiters> && ...)
inline Task<WhenAnyResult<
    std::variant<typename VoidType<typename Awaiters::return_type>::type...>>>
when_any_impl(std::index_sequence<Is...>, Awaiters &&...tasks) {
  auto self = co_await Self<>{};
  WhenAnyResult<
      std::variant<typename VoidType<typename Awaiters::return_type>::type...>>
      results{};
  Task<void> new_tasks[]{when_any_helper<Is>(tasks, results)...};
  for (auto &task : new_tasks) {
    task.handle().promise().previous_handle_ = self;
  }

  co_await WhenAnyAwaiter{new_tasks};
  co_return results;
}

template <Awaitable Awaiter>
  requires(HasReturnType<Awaiter>)
inline Task<
    WhenAnyResult<typename VoidType<typename Awaiter::return_type>::type>>
when_any(const std::vector<Awaiter> &tasks) {
  auto self = co_await Self<>{};
  WhenAnyResult<typename VoidType<typename Awaiter::return_type>::type>
      results{};
  std::vector<Task<void>> new_tasks;
  new_tasks.reserve(tasks.size());
  for (std::size_t i = 0; i < tasks.size(); ++i) {
    auto new_task = when_any_helper(tasks[i], results, i);
    new_task.handle().promise().previous_handle_ = self;
    new_tasks.emplace_back(std::move(new_task));
  }

  co_await WhenAnyAwaiter{new_tasks};
  co_return results;
}

template <Awaitable... Awaiters>
  requires(sizeof...(Awaiters) > 0) && (HasReturnType<Awaiters> && ...)
inline Task<WhenAnyResult<std::variant<typename VoidType<
    typename Awaiters::return_type>::type...>>> when_any(Awaiters &&...tasks) {
  return when_any_impl(std::make_index_sequence<sizeof...(Awaiters)>{},
                       std::forward<Awaiters>(tasks)...);
}

} // namespace co_io
