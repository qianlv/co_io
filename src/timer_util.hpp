#pragma once

#include "task.hpp"

#include "poller.hpp"

namespace co_io {

struct SleepAwaiter {

  bool await_ready() const noexcept { return false; }
  void await_suspend(std::coroutine_handle<> handle_) const noexcept {
    PollerBase::instance().add_timer(expired_time, handle_);
  }
  void await_resume() const noexcept { return; }

  std::chrono::steady_clock::time_point expired_time;
};

inline Task<void>
sleep_until(std::chrono::steady_clock::time_point expireTime) {
  co_await SleepAwaiter(expireTime);
}

inline Task<void> sleep_for(std::chrono::steady_clock::duration duration) {
  co_await SleepAwaiter(std::chrono::steady_clock::now() + duration);
}

inline Task<void> delay_run(std::chrono::steady_clock::duration duration,
                            std::function<void()> func) {
  co_await sleep_for(duration);
  func();
}

} // namespace co_io
