#pragma once

#include "concepts.hpp"
#include "task.hpp"
#include "void_type.hpp"
#include <span>

namespace co_io {

template <typename... Ts> struct WhenAllResult {
  std::tuple<Ts...> value{};
};

struct WhenAllAwaiter {
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
inline Task<void> when_all_helper(const auto &task,
                                  WhenAllResult<Ts...> &result,
                                  size_t &complete_count,
                                  std::coroutine_handle<> previous_handle) {
  using return_type = typename std::decay_t<decltype(task)>::return_type;
  if constexpr (std::is_void_v<return_type>) {
    co_await task;
  } else {
    std::get<index>(result.value) = (co_await task);
  }
  complete_count += 1;
  if (complete_count == sizeof...(Ts)) {
    auto self = co_await Self<typename Task<void>::promise_type>{};
    self.promise().previous_handle_ = previous_handle;
  }
}

template <std::size_t... Is, Awaitable... Awaiters>
  requires(HasReturnType<Awaiters> && ...)
inline Task<
    WhenAllResult<typename VoidType<typename Awaiters::return_type>::type...>>
when_all_impl(std::index_sequence<Is...>, Awaiters &&...tasks) {
  auto self = co_await Self<>{};
  WhenAllResult<typename VoidType<typename Awaiters::return_type>::type...>
      results{};
  size_t complete_count = 0;
  Task<void> new_tasks[]{
      when_all_helper<Is>(tasks, results, complete_count, self)...};
  for (auto &task : new_tasks) {
    task.handle().promise().previous_handle_ = std::noop_coroutine();
  }

  co_await WhenAllAwaiter{new_tasks};
  co_return results;
}

template <Awaitable... Awaiters>
  requires(sizeof...(Awaiters) > 0) && (HasReturnType<Awaiters> && ...)
inline Task<WhenAllResult<typename VoidType<
    typename Awaiters::return_type>::type...>> when_all(Awaiters &&...tasks) {
  return when_all_impl(std::make_index_sequence<sizeof...(Awaiters)>{},
                       std::forward<Awaiters>(tasks)...);
}

} // namespace co_io
