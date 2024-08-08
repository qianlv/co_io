#include "poller.hpp"
#include "system_call.hpp"
#include "timer_context.hpp"
namespace co_io {

SelectPoller::SelectPoller() : PollerBase() {
  FD_ZERO(&read_set_);
  FD_ZERO(&write_set_);
}

void SelectPoller::register_fd(int) { ; }

void SelectPoller::add_event(int fd, PollEvent event,
                             std::coroutine_handle<> handle) {
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

void SelectPoller::remove_event(int fd, PollEvent event) {
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

void SelectPoller::unregister_fd(int fd) {
  remove_event(fd, PollEvent::read_write());
}

void SelectPoller::poll() {

  fd_set read_set{read_set_}, write_set{write_set_};

  auto ret = detail::system_call(
      pselect(max_fd_ + 1, &read_set, &write_set, nullptr, nullptr, nullptr));

  if (ret.is_error(EINTR)) {
    return;
  }

  int n = ret.value();

  if (n > 0) {
    for (auto it = events_.begin(); it != events_.end();) {
      if (FD_ISSET(it->first, &read_set) || FD_ISSET(it->first, &write_set)) {
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

EPollPoller::EPollPoller()
    : PollerBase(), epoll_fd_(detail::system_call_value(epoll_create1(0))
                                  .execption("epoll_create1")) {}
EPollPoller::~EPollPoller() { ::close(epoll_fd_); }

void EPollPoller::register_fd(int fd) {
  struct epoll_event ev;
  ev.events = 0;
  ev.data.ptr = nullptr;
  detail::system_call_value(epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, fd, &ev))
      .execption("epoll_ctl register_fd");
}

void EPollPoller::add_event(int fd, PollEvent event,
                            std::coroutine_handle<> handle) {
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

void EPollPoller::remove_event(int fd, PollEvent) {
  struct epoll_event ev;
  ev.events = 0;
  ev.data.ptr = nullptr;
  detail::system_call_value(epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, fd, &ev))
      .execption("epoll_ctl remove_event");
}

void EPollPoller::unregister_fd(int fd) {
  detail::system_call_value(epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, fd, nullptr))
      .execption("epoll_ctl unregister_fd");
}

void EPollPoller::poll() {
  std::array<struct epoll_event, 128> events;
  auto ret = detail::system_call(
      epoll_pwait(epoll_fd_, events.data(), events.size(), -1, nullptr));

  if (ret.is_error(EINTR)) {
    return;
  }

  int n = ret.value();

  for (unsigned long i = 0; i < static_cast<unsigned long>(n); ++i) {
    auto &ev = events[i];
    if (ev.data.ptr) {
      auto handle = std::coroutine_handle<>::from_address(ev.data.ptr);
      handle.resume();
    }
  }
}

} // namespace co_io
