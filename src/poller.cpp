#include "poller.hpp"
#include "system_call.hpp"
#include <iostream>
namespace co_io {

SelectPoller::SelectPoller() : PollerBase() {
  FD_ZERO(&read_set_);
  FD_ZERO(&write_set_);
}

void SelectPoller::register_fd(int) { ; }

void SelectPoller::add_event(int fd, PollEvent event, callback handle) {
  std::cerr << "add_event " << fd << " " << event.raw() << std::endl;
  auto it = events_.find(fd);
  if (it != events_.end()) {
    it->second.event = it->second.event | event;
  } else {
    it = events_.emplace_hint(it, fd, PollerEvent{.fd = fd, .event = event});
    max_fd_ = std::max(max_fd_, fd);
  }

  if (event & PollEvent::read()) {
    FD_SET(fd, &read_set_);
    it->second.read_handle = handle;
  }
  if (event & PollEvent::write()) {
    std::cerr << "add write\n";
    FD_SET(fd, &write_set_);
    it->second.write_handle = handle;
  }
}

void SelectPoller::remove_event(int fd, PollEvent event) {
  std::cerr << "remove_event " << fd << " " << event.raw() << std::endl;
  if (auto it = events_.find(fd); it != events_.end()) {
    it->second.event = it->second.event & (~event);
    if (event & PollEvent::read()) {
      FD_CLR(fd, &read_set_);
      it->second.read_handle = {};
    }
    if (event & PollEvent::write()) {
      std::cerr << "remove write\n";
      FD_CLR(fd, &write_set_);
      it->second.write_handle = {};
    }
  }
}

void SelectPoller::unregister_fd(int fd) {
  remove_event(fd, PollEvent::read_write());
}

void SelectPoller::poll() {

  fd_set read_set{read_set_}, write_set{write_set_};

  int n = detail::system_call(pselect(max_fd_ + 1, &read_set, &write_set,
                                      nullptr, nullptr, nullptr))
              .execption("pselect");

  std::cerr << "poll " << n << std::endl;
  if (n > 0) {
    for (auto it = events_.begin(); it != events_.end();) {
      if (FD_ISSET(it->first, &read_set)) {
        it->second.read_handle();
      }
      if (FD_ISSET(it->first, &write_set)) {
        it->second.write_handle();
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
    : PollerBase(),
      epoll_fd_(detail::Execpted(epoll_create1(0)).execption("epoll_create1")) {
}
EPollPoller::~EPollPoller() { ::close(epoll_fd_); }

void EPollPoller::register_fd(int fd) {
  struct epoll_event ev;
  ev.events = 0;
  ev.data.ptr = nullptr;
  detail::Execpted(epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, fd, &ev))
      .execption("epoll_ctl register_fd");
}

void EPollPoller::add_event(int fd, PollEvent event, callback handle) {
  struct epoll_event ev;
  ev.events = EPOLLERR | EPOLLET;
  if ((event & PollEvent::read()).raw() > 0) {
    ev.events |= EPOLLIN;
  }
  if ((event & PollEvent::write()).raw() > 0) {
    ev.events |= EPOLLOUT;
  }
  ev.data.fd = fd;

  detail::Execpted(epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, fd, &ev))
      .execption("epoll_ctl add_event");
  handles_.insert_or_assign(fd, std::move(handle));
}

void EPollPoller::remove_event(int fd, PollEvent) {
  struct epoll_event ev;
  ev.events = 0;
  ev.data.ptr = nullptr;
  detail::Execpted(epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, fd, &ev))
      .execption("epoll_ctl remove_event");
  handles_.erase(fd);
}

void EPollPoller::unregister_fd(int fd) {
  detail::Execpted(epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, fd, nullptr))
      .execption("epoll_ctl unregister_fd");
  handles_.erase(fd);
}

void EPollPoller::poll() {
  int n = detail::system_call(epoll_pwait(epoll_fd_, events_.data(),
                                          static_cast<int>(events_.size()), -1,
                                          nullptr))
              .execption("epoll_pwait");

  for (unsigned long i = 0; i < static_cast<unsigned long>(n); ++i) {
    auto &ev = events_[i];
    if (auto it = handles_.find(ev.data.fd); it != handles_.end()) {
      it->second();
    }
  }
}

} // namespace co_io
