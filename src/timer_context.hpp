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
      : clock_fd_(Execpted<int>(::timerfd_create(CLOCK_MONOTONIC, 0))
                      .execption("timerfd_create"),
                  loop) {
    run_task(poll_timer());
  }

  uint32_t add_timer(std::chrono::steady_clock::time_point expired_time,
                     std::coroutine_handle<> coroutine);
  // uint32_t add_timer(std::chrono::steady_clock::time_point expired_time,
  //                    std::function<void()> callback);

  void cancel_timer(uint64_t id);

  Task<void> sleep_until(std::chrono::steady_clock::time_point expireTime);
  Task<void> sleep_for(std::chrono::steady_clock::duration duration);
  Task<void> delay_run(std::chrono::steady_clock::duration duration,
                       std::function<void()> func);

private:
  struct TimerEntry {
    std::chrono::steady_clock::time_point expired_time;
    std::coroutine_handle<> callback;
    uint64_t id;

    bool operator<(const TimerEntry &other) const noexcept {
      return expired_time >= other.expired_time;
    }
  };

  struct TimerPromise : public Promise<void> {
    TimerContext *timer_context;
    uint32_t timer_id_;
    bool completed = false;

    auto get_return_object() {
      return std::coroutine_handle<TimerPromise>::from_promise(*this);
    }
    inline ~TimerPromise() {
      if (!completed) {
        timer_context->cancel_timer(timer_id_);
      }
    }

    PollerPromise &operator=(PollerPromise &&) = delete;
  };

  struct SleepAwaiter {
    bool await_ready() const noexcept { return false; }
    void await_suspend(std::coroutine_handle<TimerPromise> h) noexcept {
      handle_ = h;
      auto &promise = h.promise();
      promise.timer_context = timer_context_;
      promise.timer_id_ = timer_context_->add_timer(expired_time_, h);
    }
    void await_resume() const noexcept { handle_.promise().completed = true; }

    std::chrono::steady_clock::time_point expired_time_;
    TimerContext *timer_context_;
    std::coroutine_handle<TimerPromise> handle_{};
  };

  inline Task<void, TimerPromise>
  waiting_for_timer(std::chrono::steady_clock::time_point expireTime) {
    co_await SleepAwaiter{expireTime, this};
  }

  void reset();

  Task<void> poll_timer();

  std::priority_queue<TimerEntry> timers_;
  std::unordered_set<uint64_t> cancel_timers_;
  AsyncFile clock_fd_;
  uint32_t next_timer_id_ = 0;
};

using TimerContextPtr = std::unique_ptr<TimerContext>;

} // namespace co_io
