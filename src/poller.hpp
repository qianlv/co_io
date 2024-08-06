#pragma once

#include <coroutine>
#include <spdlog/spdlog.h>
#include <sys/select.h>
#include <unordered_map>

#include "system_call.hpp"

namespace co_io {

class PollEvent {
private:
  unsigned int event;
  constexpr PollEvent(unsigned int event) : event(event) {}
  constexpr PollEvent() : event(0) {}

public:
  constexpr PollEvent operator&(PollEvent rhs) const {
    return event & rhs.event;
  }

  constexpr operator bool() const { return event != 0; }

  constexpr PollEvent operator|(PollEvent rhs) const {
    return PollEvent(event | rhs.event);
  }

  constexpr PollEvent operator^(PollEvent rhs) const {
    return PollEvent(event ^ rhs.event);
  }

  bool operator==(PollEvent rhs) const { return event == rhs.event; }

  constexpr PollEvent operator~() const { return PollEvent(~event); }

  static constexpr PollEvent none() { return PollEvent(); }
  static constexpr PollEvent read() { return PollEvent(1 << 0); }
  static constexpr PollEvent write() { return PollEvent(1 << 1); }
  static constexpr PollEvent read_write() { return read() | write(); }
  static constexpr PollEvent read_remove() { return PollEvent(1 << 2); }
  static constexpr PollEvent write_remove() { return PollEvent(1 << 3); }
  static constexpr PollEvent read_write_remove() {
    return read_remove() | write_remove();
  }

  constexpr unsigned int raw() const { return event; }
};

class PollerBase {
public:
  struct PollerEvent {
    int fd;
    PollEvent event;
    std::coroutine_handle<> handle;
  };

  virtual void register_fd(int fd) = 0;

  virtual void update_fd(int fd, PollEvent event,
                         std::coroutine_handle<> handle) = 0;

  virtual void poll() = 0;

  virtual ~PollerBase() = default;
};

class SelectPolloer : public PollerBase {
public:
  SelectPolloer() {
    FD_ZERO(&read_set_);
    FD_ZERO(&write_set_);
  }

  void register_fd(int) override { ; }

  void update_fd(int fd, PollEvent event,
                 std::coroutine_handle<> handle) override {
    spdlog::debug("update fd: {}, event: {}, handle: {}", fd, event.raw(),
                  handle.address());
    auto it = events_.find(fd);
    if ((event & PollEvent::read()) || (event & PollEvent::write())) {
      if (it == events_.end()) {
        events_.emplace_hint(
            it, fd, PollerEvent{.fd = fd, .event = event, .handle = handle});
      } else {
        it->second.event = it->second.event | event;
        it->second.handle = handle;
      }

      if (event & PollEvent::read()) {
        FD_SET(fd, &read_set_);
      }
      if (event & PollEvent::write()) {
        FD_SET(fd, &write_set_);
      }
      max_fd_ = std::max(max_fd_, fd);
    } else if (it != events_.end()) {
      if (event & PollEvent::read_remove()) {
        spdlog::debug("read_remove: {}", it->second.event.raw());
        it->second.event = it->second.event & (~PollEvent::read());
        FD_CLR(fd, &read_set_);
      }
      if (event & PollEvent::write_remove()) {
        spdlog::debug("write_remove: {}", it->second.event.raw());
        it->second.event = it->second.event & (~PollEvent::write());
        FD_CLR(fd, &write_set_);
      }
    }
  }

  void poll() override {
    spdlog::debug("poll... {} {}", events_.size(), max_fd_);

    fd_set read_set{read_set_}, write_set{write_set_};

    int n = detail::system_call(
                pselect(max_fd_ + 1, &read_set, &write_set, nullptr, nullptr, nullptr))
                .execption("select");

    spdlog::debug("select return: {}", n);

    if (n > 0) {
      for (auto it = events_.begin(); it != events_.end();) {
        if (FD_ISSET(it->first, &read_set) || FD_ISSET(it->first, &write_set)) {
          spdlog::debug("select fd = {} handle {} resume", it->first,
                        it->second.handle.address());
          it->second.handle.resume();
        }

        if (it->second.event == PollEvent::none()) {
          if (max_fd_ == it->first) {
            max_fd_ = -1;
          }
          it = events_.erase(it);
        } else {
          ++it;
        }
      }

      if (max_fd_ == -1) {
        for (auto [fd, _] : events_) {
          max_fd_ = std::max(max_fd_, fd);
        }
      }
    }
  }

private:
  fd_set read_set_;
  fd_set write_set_;
  int max_fd_ = 0;
  std::unordered_map<int, PollerEvent> events_;
};

} // namespace co_io
