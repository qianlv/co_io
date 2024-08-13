#include "async_file.hpp"
#include "loop.hpp"
#include "system_call.hpp"
#include <iostream>

namespace co_io {

AsyncFile::AsyncFile(int fd, LoopBase *loop, unsigned time_out_sec)
    : FileDescriptor(fd), loop_(loop), time_out_sec_(time_out_sec) {
  auto flags = detail::system_call(fcntl(fd, F_GETFL)).execption("fcntl");
  detail::system_call(fcntl(fd, F_SETFL, flags | O_NONBLOCK))
      .execption("fcntl");
  loop_->poller()->register_fd(fd);
}

AsyncFile::FinalAwaiter<detail::Execpted<ssize_t>>
AsyncFile::async_read(void *buf, size_t size) {
  auto task = async_r<ssize_t>([fd = this->fd(), buf, size]() {
    return detail::system_call(::read(fd, buf, size));
  }, PollEvent::read());
  auto task_handle = task.get_handle();
  uint32_t timer_id = 0;
  bool has_timer = false;
  if (time_out_sec_ > 0) {
    has_timer = true;
    timer_id = loop_->timer()->add_timer(
        std::chrono::steady_clock::now() + std::chrono::seconds(time_out_sec_),
        [fd = this->fd(), loop = this->loop_, task_handle] {
          task_handle.promise().is_timeout = true;
          loop->poller()->remove_event(fd, PollEvent::read());
          task_handle.resume();
        });
  }

  loop_->poller()->add_event(
      this->fd(), PollEvent::read(),
      [timer_id, has_timer, task_handle, loop = this->loop_] {
        if (has_timer) {
          loop->timer()->cancel_timer(timer_id);
        }
        task_handle.resume();
      });
  return FinalAwaiter<detail::Execpted<ssize_t>>{std::move(task)};
}

AsyncFile::FinalAwaiter<detail::Execpted<ssize_t>>
AsyncFile::async_read(ByteBuffer &buf) {
  return async_read(buf.data(), buf.size());
}

AsyncFile::FinalAwaiter<detail::Execpted<ssize_t>>
AsyncFile::async_write(const void *buf, size_t size) {
  auto task = async_r<ssize_t>([fd = this->fd(), buf, size]() {
    return detail::system_call(::write(fd, buf, size));
  }, PollEvent::write());

  std::cerr << "async_write " << PollEvent::write().raw() << " " << PollEvent::read().raw() << std::endl;
  loop_->poller()->add_event(this->fd(), PollEvent::write(), task.get_handle());
  return FinalAwaiter<detail::Execpted<ssize_t>>{std::move(task)};
}

AsyncFile::FinalAwaiter<detail::Execpted<ssize_t>>
AsyncFile::async_write(std::string_view buf) {
  return async_write(buf.data(), buf.size());
}

AsyncFile::FinalAwaiter<detail::Execpted<int>>
AsyncFile::async_accept(AddressSolver::Address &) {
  auto task = async_r<int>([fd = this->fd()]() {
    return detail::system_call(::accept(fd, nullptr, nullptr));
  }, PollEvent::read());
  loop_->poller()->add_event(this->fd(), PollEvent::read(), task.get_handle());
  return FinalAwaiter<detail::Execpted<int>>{std::move(task)};
}

AsyncFile::FinalAwaiter<detail::Execpted<int>>
AsyncFile::async_connect(AddressSolver::Address const &addr) {
  auto task = async_r<int>([fd = this->fd(), &addr]() {
    return detail::system_call(::connect(fd, &addr.addr_, addr.len_));
  }, PollEvent::write());
  loop_->poller()->add_event(this->fd(), PollEvent::write(), task.get_handle());
  return FinalAwaiter<detail::Execpted<int>>{std::move(task)};
}

AsyncFile AsyncFile::bind(AddressSolver::AddressInfo const &addr,
                          LoopBase *loop) {
  auto async_file = AsyncFile(addr.create_socket(), loop);
  AddressSolver::Address serve = addr.get_address();
  int on = 1;
  detail::system_call(
      setsockopt(async_file.fd(), SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)))
      .execption("setsockopt");
  detail::system_call(
      setsockopt(async_file.fd(), SOL_SOCKET, SO_REUSEPORT, &on, sizeof(on)))
      .execption("setsockopt");
  detail::system_call(::bind(async_file.fd(), &serve.addr_, serve.len_))
      .execption("bind");
  detail::system_call(::listen(async_file.fd(), SOMAXCONN)).execption("listen");
  return async_file;
}

template <typename Ret>
AsyncFile::TaskAsnyc<detail::Execpted<Ret>>
AsyncFile::async_r(std::function<detail::Execpted<Ret>()> func, PollEvent event) {
  while (true) {
    auto ret = co_await func;
    if (!ret.is_nonblocking_error()) {
      loop_->poller()->remove_event(fd(), event);
      co_return ret;
    }
  }
}

AsyncFile::~AsyncFile() {
  if (fd() != -1) {
    std::cerr << "~AsyncFile() " << fd() << std::endl;
    loop_->poller()->unregister_fd(fd());
  }
}

} // namespace co_io
