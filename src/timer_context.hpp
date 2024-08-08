#pragma once

#include <chrono>
#include <coroutine>
#include <fcntl.h>
#include <queue>
#include <sys/timerfd.h>

#include "async_file.hpp"
#include "poller.hpp"
#include "system_call.hpp"
#include "task.hpp"

namespace co_io {

class TimerContext;
using TimerContextPtr = std::shared_ptr<TimerContext>;

class TimerContext : public std::enable_shared_from_this<TimerContext> {
  struct SleepAwaiter;

public:
  TimerContext(PollerBasePtr poller)
      : clock_fd_(
            detail::system_call_value<int>(::timerfd_create(CLOCK_MONOTONIC, 0))
                .execption("timerfd_create"),
            poller),
        poller_(poller) {
    poll_timer();
  }

  void add_timer(std::chrono::steady_clock::time_point expired_time,
                 std::coroutine_handle<> coroutine);

  SleepAwaiter sleep_until(std::chrono::steady_clock::time_point expireTime);
  SleepAwaiter sleep_for(std::chrono::steady_clock::duration duration);
  TaskNoSuspend<void> delay_run(std::chrono::steady_clock::duration duration,
                                std::function<void()> func);

private:
  struct TimerEntry {
    std::chrono::steady_clock::time_point expired_time;
    std::coroutine_handle<> handle_;

    bool operator<(const TimerEntry &other) const noexcept {
      return expired_time >= other.expired_time;
    }
  };

  struct SleepAwaiter {
    bool await_ready() const noexcept { return false; }
    void await_suspend(std::coroutine_handle<> handle_) const noexcept {
      timer_context->add_timer(expired_time, handle_);
    }
    void await_resume() const noexcept { return; }

    std::chrono::steady_clock::time_point expired_time;
    TimerContextPtr timer_context;
  };

  void reset();

  TaskNoSuspend<void> poll_timer();

  std::priority_queue<TimerEntry> timers_;
  AsyncFile clock_fd_;
  PollerBasePtr poller_;
};

} // namespace co_io
