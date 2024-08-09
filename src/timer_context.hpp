#pragma once

#include <chrono>
#include <coroutine>
#include <fcntl.h>
#include <queue>
#include <sys/timerfd.h>
#include <unordered_set>

#include "async_file.hpp"
#include "task.hpp"

namespace co_io {

class TimerContext;
class LoopBase;

class TimerContext {
  struct SleepAwaiter;

public:
  TimerContext(LoopBase *loop)
      : clock_fd_(
            detail::system_call_value<int>(::timerfd_create(CLOCK_MONOTONIC, 0))
                .execption("timerfd_create"),
            loop),
        loop_(loop) {
    poll_timer();
  }

  uint32_t add_timer(std::chrono::steady_clock::time_point expired_time,
                 std::coroutine_handle<> coroutine);
  uint32_t add_timer(std::chrono::steady_clock::time_point expired_time,
                 std::function<void()> callback);

  void cancel_timer(uint64_t id);

  SleepAwaiter sleep_until(std::chrono::steady_clock::time_point expireTime);
  SleepAwaiter sleep_for(std::chrono::steady_clock::duration duration);
  TaskNoSuspend<void> delay_run(std::chrono::steady_clock::duration duration,
                                std::function<void()> func);

private:
  struct TimerEntry {
    std::chrono::steady_clock::time_point expired_time;
    std::function<void()> callback;
    uint64_t id;

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
    TimerContext *timer_context;
  };

  void reset();

  TaskNoSuspend<void> poll_timer();

  std::priority_queue<TimerEntry> timers_;
  std::unordered_set<uint64_t> cancel_timers_;
  AsyncFile clock_fd_;
  LoopBase *loop_;
  uint32_t next_timer_id_ = 0;
};

using TimerContextPtr = std::unique_ptr<TimerContext>;

} // namespace co_io
