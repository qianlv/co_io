#include "timer_context.hpp"
#include "poller.hpp"
#include <spdlog/spdlog.h>

namespace co_io {

void TimerContext::add_timer(std::chrono::steady_clock::time_point expired_time,
                             std::coroutine_handle<> coroutine) {
  bool is_reset =
      (timers_.empty() || expired_time < timers_.top().expired_time);
  timers_.emplace(expired_time, coroutine);
  if (is_reset) {
    reset();
  }
}

void TimerContext::reset() {
  if (timers_.empty()) {
    return;
  }

  spdlog::debug("TimerContext::reset() {}", timers_.size());
  struct itimerspec new_value;
  auto top = timers_.top();
  std::chrono::nanoseconds nanos =
      top.expired_time - std::chrono::steady_clock::now();
  new_value.it_value.tv_sec = nanos.count() / 1000'000'000l;
  new_value.it_value.tv_nsec = nanos.count() % 1000'000'000l;
  new_value.it_interval.tv_sec = 0;
  new_value.it_interval.tv_nsec = 0;

  spdlog::debug("TimerContext::reset() timer {} {}", new_value.it_value.tv_sec,
                new_value.it_value.tv_nsec);

  detail::system_call_value<int>(
      ::timerfd_settime(clock_fd_, 0, &new_value, nullptr))
      .execption("timerfd_settime");

  if (!register_) {
    PollerBase::instance().register_fd(fd());
    register_ = true;
  }

  spdlog::debug("TimerContext::reset() done = {}", poll_done_);
  if (poll_done_) {
    poll_done_ = false;
    poll_task_ = poll_timer();
    spdlog::debug("TimerContext::reset() add_event {}",
                  poll_task_.get_handle().address());
    PollerBase::instance().add_event(clock_fd_, PollEvent::read(),
                                     poll_task_.get_handle());
  }
}

Task<void> TimerContext::poll_timer() {

  co_await SupsendAwaiter{};

  spdlog::debug("TimerContext::poll_timer() start");

  uint64_t exp = 0;
  detail::system_call_value<ssize_t>(::read(clock_fd_, &exp, sizeof(exp)))
      .execption("read poller");

  while (!timers_.empty() &&
         timers_.top().expired_time <= std::chrono::steady_clock::now()) {
    spdlog::debug("TimerContext::poll_timer() pop");
    auto top = timers_.top();
    timers_.pop();
    top.handle_.resume();
  }
  spdlog::debug("TimerContext::poll_timer() end");

  poll_done_ = true;
  reset();
  co_return;
}

} // namespace co_io
