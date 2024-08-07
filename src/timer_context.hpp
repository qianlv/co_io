#pragma once
#include <chrono>
#include <coroutine>
#include <fcntl.h>
#include <queue>
#include <sys/timerfd.h>

#include "system_call.hpp"
#include "task.hpp"

namespace co_io {

class TimerContext {
public:
  TimerContext()
      : clock_fd_(
            detail::system_call_value<int>(::timerfd_create(CLOCK_MONOTONIC, 0))
                .execption("timerfd_create")) {
    auto flags =
        detail::system_call(fcntl(clock_fd_, F_GETFL)).execption("fcntl");
    detail::system_call(fcntl(clock_fd_, F_SETFL, flags | O_NONBLOCK))
        .execption("fcntl");
  }

  void add_timer(std::chrono::steady_clock::time_point expired_time,
                 std::coroutine_handle<> coroutine);

  int fd() const { return clock_fd_; }

private:
  struct TimerEntry {
    std::chrono::steady_clock::time_point expired_time;
    std::coroutine_handle<> handle_;

    bool operator<(const TimerEntry &other) const noexcept {
      return expired_time >= other.expired_time;
    }
  };

  void reset();

  Task<void> poll_timer();

  std::priority_queue<TimerEntry> timers_;
  int clock_fd_;
  bool register_{false};
  bool is_task_done_ {true};
  Task<void> poll_done_;
  Task<void> poll_task_;
};

} // namespace co_io
