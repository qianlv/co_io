#include "timer_context.hpp"
#include "task.hpp"
#include <iostream>

namespace co_io {

uint32_t
TimerContext::add_timer(std::chrono::steady_clock::time_point expired_time,
                        std::coroutine_handle<> coroutine) {
  return add_timer(expired_time, [coroutine]() { coroutine.resume(); });
}

uint32_t
TimerContext::add_timer(std::chrono::steady_clock::time_point expired_time,
                        std::function<void()> callback) {

  bool is_reset =
      (timers_.empty() || expired_time < timers_.top().expired_time);
  timers_.emplace(expired_time, std::move(callback), ++next_timer_id_);
  if (is_reset) {
    reset();
  }
  return next_timer_id_;
}

void TimerContext::cancel_timer(uint64_t id) { cancel_timers_.insert(id); }

void TimerContext::reset() {
  while (!cancel_timers_.empty() && !timers_.empty()) { // remove cancel timer
    if (auto it = cancel_timers_.find(timers_.top().id);
        it != cancel_timers_.end()) {
      timers_.pop();
      cancel_timers_.erase(it);
    } else {
      break;
    }
  }

  if (timers_.empty()) {
    return;
  }

  struct itimerspec new_value;
  auto top = timers_.top();
  std::chrono::nanoseconds nanos =
      top.expired_time - std::chrono::steady_clock::now();
  auto count = nanos.count();
  if (count < 0) { // just set 1 nanosecond to trigger
    count = 1;
  }

  new_value.it_value.tv_sec = count / 1000'000'000l;
  new_value.it_value.tv_nsec = count % 1000'000'000l;
  new_value.it_interval.tv_sec = 0;
  new_value.it_interval.tv_nsec = 0;

  Execpted<int>(::timerfd_settime(clock_fd_.fd(), 0, &new_value, nullptr))
      .execption("timerfd_settime");
}

Task<void> TimerContext::poll_timer() {
  while (true) {
    uint64_t exp = 0;
    co_await clock_fd_.async_read(reinterpret_cast<void *>(&exp), sizeof(exp));

    while (!timers_.empty() &&
           timers_.top().expired_time <= std::chrono::steady_clock::now()) {
      auto top = timers_.top();
      timers_.pop();
      if (cancel_timers_.find(top.id) != cancel_timers_.end()) {
        cancel_timers_.erase(top.id);
        continue;
      }
      top.callback();
    }
    reset();
  }
}

TimerContext::SleepAwaiter
TimerContext::sleep_until(std::chrono::steady_clock::time_point expireTime) {
  return SleepAwaiter(expireTime, this);
}

TimerContext::SleepAwaiter
TimerContext::sleep_for(std::chrono::steady_clock::duration duration) {
  return SleepAwaiter(std::chrono::steady_clock::now() + duration, this);
}

Task<void>
TimerContext::delay_run(std::chrono::steady_clock::duration duration,
                        std::function<void()> func) {
  co_await sleep_for(duration);
  func();
}

} // namespace co_io
