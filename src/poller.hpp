#pragma once

#include <coroutine>
#include <spdlog/spdlog.h>
#include <sys/epoll.h>
#include <sys/select.h>
#include <unordered_map>

#include "system_call.hpp"
#include "timer_context.hpp"

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

  constexpr unsigned int raw() const { return event; }
};

class PollerBase {
public:
  PollerBase() { g_instance = this; }

  struct PollerEvent {
    int fd;
    PollEvent event;
    std::coroutine_handle<> handle;
  };

  virtual void register_fd(int fd) = 0;
  virtual void unregister_fd(int fd) = 0;

  virtual void add_event(int fd, PollEvent event,
                         std::coroutine_handle<> handle) = 0;
  virtual void remove_event(int fd, PollEvent event) = 0;

  virtual void poll() = 0;

  void add_timer(std::chrono::steady_clock::time_point expired_time,
                 std::coroutine_handle<> coroutine) {
    timer_context_.add_timer(expired_time, coroutine);
  }

  void add_timer(std::chrono::steady_clock::duration duration,
                 std::coroutine_handle<> coroutine) {
    timer_context_.add_timer(std::chrono::steady_clock::now() + duration,
                             coroutine);
  }

  virtual ~PollerBase() { g_instance = nullptr; }

  static inline thread_local PollerBase *g_instance = nullptr;

  static PollerBase &instance() {
    assert(g_instance);
    return *g_instance;
  }

protected:
  TimerContext timer_context_;
};

class SelectPoller : public PollerBase {
public:
  SelectPoller() : PollerBase() {
    FD_ZERO(&read_set_);
    FD_ZERO(&write_set_);
  }

  void register_fd(int) override { ; }

  void add_event(int fd, PollEvent event,
                 std::coroutine_handle<> handle) override {
    spdlog::debug("update fd: {}, event: {}, handle: {}", fd, event.raw(),
                  handle.address());
    if (auto it = events_.find(fd); it != events_.end()) {
      it->second.event = it->second.event | event;
      it->second.handle = handle;
    } else {
      events_.emplace_hint(
          it, fd, PollerEvent{.fd = fd, .event = event, .handle = handle});
      max_fd_ = std::max(max_fd_, fd);
    }

    if (event & PollEvent::read()) {
      FD_SET(fd, &read_set_);
    }
    if (event & PollEvent::write()) {
      FD_SET(fd, &write_set_);
    }
  }

  void remove_event(int fd, PollEvent event) override {
    spdlog::debug("remove fd: {}, event: {}", fd, event.raw());
    if (auto it = events_.find(fd); it != events_.end()) {
      it->second.event = it->second.event & (~event);
      if (event & PollEvent::read()) {
        FD_CLR(fd, &read_set_);
      }
      if (event & PollEvent::write()) {
        FD_CLR(fd, &write_set_);
      }
    }
  }

  void unregister_fd(int fd) override {
    remove_event(fd, PollEvent::read_write());
  }

  void poll() override {
    spdlog::debug("poll... {} {}", events_.size(), max_fd_);

    fd_set read_set{read_set_}, write_set{write_set_};

    int n = detail::system_call(pselect(max_fd_ + 1, &read_set, &write_set,
                                        nullptr, nullptr, nullptr))
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

class EPollPoller : public PollerBase {
public:
  EPollPoller()
      : PollerBase(), epoll_fd_(detail::system_call_value(epoll_create1(0))
                                    .execption("epoll_create1")) {}
  ~EPollPoller() override { ::close(epoll_fd_); }

  void register_fd(int fd) override {
    struct epoll_event ev;
    ev.events = 0;
    ev.data.ptr = nullptr;
    detail::system_call_value(epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, fd, &ev))
        .execption("epoll_ctl register_fd");
  }

  void add_event(int fd, PollEvent event,
                 std::coroutine_handle<> handle) override {
    spdlog::debug("update fd: {}, event: {}, handle: {}", fd, event.raw(),
                  handle.address());
    struct epoll_event ev;
    ev.events = EPOLLERR | EPOLLET;
    if (event & PollEvent::read()) {
      ev.events |= EPOLLIN;
    }
    if (event & PollEvent::write()) {
      ev.events |= EPOLLOUT;
    }
    ev.data.ptr = handle.address();

    detail::system_call_value(epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, fd, &ev))
        .execption("epoll_ctl add_event");
  }

  void remove_event(int fd, PollEvent event) override {
    spdlog::debug("remove fd: {}, event: {}", fd, event.raw());
    struct epoll_event ev;
    ev.events = 0;
    ev.data.ptr = nullptr;
    detail::system_call_value(epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, fd, &ev))
        .execption("epoll_ctl remove_event");
  }

  void unregister_fd(int fd) override {
    detail::system_call_value(epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, fd, nullptr))
        .execption("epoll_ctl unregister_fd");
  }

  void poll() override {
    spdlog::debug("poll...");
    std::array<struct epoll_event, 128> events;
    int n = detail::system_call(epoll_pwait(epoll_fd_, events.data(),
                                            events.size(), -1, nullptr))
                .execption("epoll_wait");
    spdlog::debug("epoll_wait return: {}", n);
    for (unsigned long i = 0; i < static_cast<unsigned long>(n); ++i) {
      auto &ev = events[i];
      auto handle = std::coroutine_handle<>::from_address(ev.data.ptr);
      handle.resume();
    }
  }

private:
  int epoll_fd_;
};

} // namespace co_io
