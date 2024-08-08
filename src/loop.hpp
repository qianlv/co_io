#pragma once

#include "poller.hpp"
#include "timer_context.hpp"

namespace co_io {

class LoopBase {
public:
  virtual ~LoopBase() = default;
  virtual void run() = 0;
  virtual void stop() = 0;
  virtual PollerBasePtr poller() const = 0;
  virtual TimerContextPtr timer() const = 0;
};

template <typename POLLER> class Loop : public LoopBase {
public:
  Loop(size_t count = 0);

  PollerBasePtr poller() const override { return poller_; }
  TimerContextPtr timer() const override { return timer_; }

  void run() override {
    size_t i = 0;
    while ((count_ == 0 || i < count_) && !stop_) {
      poller_->poll();
      i += 1;
    }
  }

  void stop() override { stop_ = true; }

private:
  PollerBasePtr poller_;
  TimerContextPtr timer_;
  bool stop_{false};
  size_t count_{0};
};

template <typename POLLER>
Loop<POLLER>::Loop(size_t count)
    : poller_(std::make_shared<POLLER>()),
      timer_(std::make_shared<TimerContext>(poller_)), count_{count} {}

using EPollLoop = Loop<EPollPoller>;
using SelectLoop = Loop<SelectPoller>;

} // namespace co_io
