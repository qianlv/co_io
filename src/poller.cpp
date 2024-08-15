#include "poller.hpp"
#include "system_call.hpp"
#include <iostream>
namespace co_io {

void PollerBase::register_fd(int fd) {
  handles_.insert_or_assign(fd,
                            PollerEvent{.fd = fd, .event = PollEvent::none()});
}

void PollerBase::unregister_fd(int fd) { handles_.erase(fd); }

void PollerBase::add_event(int fd, PollEvent event, callback handle) {
  auto it = handles_.find(fd);
  if (it != handles_.end()) {
    it->second.event = it->second.event | event;
  } else {
    it = handles_.emplace_hint(it, fd, PollerEvent{.fd = fd, .event = event});
  }
  if (event & PollEvent::read()) {
    it->second.read_handle = handle;
  }
  if (event & PollEvent::write()) {
    it->second.write_handle = handle;
  }
}

bool PollerBase::remove_event(int fd, PollEvent event) {
  if (auto it = handles_.find(fd); it != handles_.end()) {
    it->second.event = it->second.event & (~event);
    if (event & PollEvent::read()) {
      it->second.read_handle = {};
    }
    if (event & PollEvent::write()) {
      it->second.write_handle = {};
    }
    return true;
  }
  return false;
}

SelectPoller::SelectPoller() : PollerBase() {
  FD_ZERO(&read_set_);
  FD_ZERO(&write_set_);
}

void SelectPoller::register_fd(int) { ; }

void SelectPoller::unregister_fd(int fd) {
  remove_event(fd, PollEvent::read() | PollEvent::write());
}

void SelectPoller::add_event(int fd, PollEvent event, callback handle) {
  // std::cerr << "add_event " << fd << " " << event.raw() << std::endl;
  PollerBase::add_event(fd, event, handle);
  if (handles_.at(fd).event & PollEvent::read()) {
    FD_SET(fd, &read_set_);
  }
  if (handles_.at(fd).event & PollEvent::write()) {
    FD_SET(fd, &write_set_);
  }
  max_fd_ = std::max(max_fd_, fd);
}

bool SelectPoller::remove_event(int fd, PollEvent event) {
  // std::cerr << "remove_event " << fd << " " << event.raw() << std::endl;
  if (PollerBase::remove_event(fd, event)) {
    if (event & PollEvent::read()) {
      FD_CLR(fd, &read_set_);
    }
    if (event & PollEvent::write()) {
      FD_CLR(fd, &write_set_);
    }
    return true;
  }
  return false;
}

void SelectPoller::poll() {

  fd_set read_set{read_set_}, write_set{write_set_};

  int n = detail::system_call(pselect(max_fd_ + 1, &read_set, &write_set,
                                      nullptr, nullptr, nullptr))
              .execption("pselect");

  if (n > 0) {
    for (auto it = handles_.begin(); it != handles_.end();) {
      int fd = it->first;
      if (FD_ISSET(fd, &read_set) && it->second.read_handle) {
        it->second.read_handle();
      }
      if (FD_ISSET(fd, &write_set) && it->second.write_handle) {
        it->second.write_handle();
      }

      if (!it->second.read_handle && !it->second.write_handle) {
        it = handles_.erase(it);
        if (max_fd_ == fd) {
          max_fd_ = -1;
        }
      } else {
        ++it;
      }
    }

    if (max_fd_ == -1) {
      for (auto [fd, _] : handles_) {
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
  PollerBase::register_fd(fd);

  struct epoll_event ev;
  ev.events = 0;
  ev.data.ptr = nullptr;
  detail::Execpted(epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, fd, &ev))
      .execption("epoll_ctl register_fd");
}

void EPollPoller::add_event(int fd, PollEvent event, callback handle) {
  // std::cerr << "add_event " << fd << " " << event.raw() << std::endl;
  PollerBase::add_event(fd, event, handle);
  struct epoll_event ev;
  ev.events = EPOLLERR | EPOLLET | EPOLLONESHOT;
  if (handles_.at(fd).event & PollEvent::read()) {
    ev.events |= EPOLLIN;
  }
  if (handles_.at(fd).event & PollEvent::write()) {
    ev.events |= EPOLLOUT;
  }
  ev.data.fd = fd;

  detail::Execpted(epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, fd, &ev))
      .execption("epoll_ctl add_event");
}

bool EPollPoller::remove_event(int fd, PollEvent event) {
  // std::cerr << "remove_event " << fd << " " << event.raw() << std::endl;
  if (PollerBase::remove_event(fd, event)) {
    struct epoll_event ev;
    ev.events = EPOLLERR | EPOLLET | EPOLLONESHOT;
    if (handles_.at(fd).event & PollEvent::read()) {
      ev.events |= EPOLLIN;
    }
    if (handles_.at(fd).event & PollEvent::write()) {
      ev.events |= EPOLLOUT;
    }
    ev.data.fd = fd;
    detail::Execpted(epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, fd, &ev))
        .execption("epoll_ctl remove_event");
    return true;
  }
  return false;
}

void EPollPoller::unregister_fd(int fd) {
  PollerBase::unregister_fd(fd);
  detail::Execpted(epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, fd, nullptr))
      .execption("epoll_ctl unregister_fd");
}

void EPollPoller::poll() {
  int n = detail::system_call(epoll_pwait(epoll_fd_, events_.data(),
                                          static_cast<int>(events_.size()), -1,
                                          nullptr))
              .execption("epoll_pwait");

  for (unsigned long i = 0; i < static_cast<unsigned long>(n); ++i) {
    auto &ev = events_[i];
    if (auto it = handles_.find(ev.data.fd); it != handles_.end()) {
      if (ev.events & EPOLLOUT && it->second.write_handle) {
        it->second.write_handle();
      }
      if (ev.events & EPOLLIN && it->second.read_handle) {
        it->second.read_handle();
      }
    }
  }
}

} // namespace co_io
