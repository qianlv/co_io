#pragma once

#include "concepts.hpp"
#include "task.hpp"
#include "void_type.hpp"
#include <span>

namespace co_io {

template <typename... Ts> struct WhenAnyResult {
  std::variant<Ts...> value{};
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

template <size_t index, typename... Ts>
inline Task<void> when_any_helper(const auto &task,
                                  WhenAnyResult<Ts...> &result) {
  using return_type = typename std::decay_t<decltype(task)>::return_type;
  if constexpr (std::is_void_v<return_type>) {
    co_await task;
  } else {
    result.value.template emplace<index>(co_await task);
  }
  result.index = index;
}

template <std::size_t... Is, Awaitable... Awaiters>
  requires(HasReturnType<Awaiters> && ...)
inline Task<
    WhenAnyResult<typename VoidType<typename Awaiters::return_type>::type...>>
when_any_impl(std::index_sequence<Is...>, Awaiters &&...tasks) {
  auto self = co_await Self<>{};
  WhenAnyResult<typename VoidType<typename Awaiters::return_type>::type...>
      results{};
  Task<void> new_tasks[]{when_any_helper<Is>(tasks, results)...};
  for (auto &task : new_tasks) {
    task.handle().promise().previous_handle_ = self;
  }

  co_await WhenAnyAwaiter{new_tasks};
  co_return results;
}

template <Awaitable... Awaiters>
  requires(sizeof...(Awaiters) > 0) && (HasReturnType<Awaiters> && ...)
inline Task<WhenAnyResult<typename VoidType<
    typename Awaiters::return_type>::type...>> when_any(Awaiters &&...tasks) {
  return when_any_impl(std::make_index_sequence<sizeof...(Awaiters)>{},
                       std::forward<Awaiters>(tasks)...);
}

} // namespace co_io
