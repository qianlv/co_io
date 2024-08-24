#include "async_file.hpp"
#include "loop.hpp"
#include "system_call.hpp"

namespace co_io {


AsyncFile::AsyncFile(int fd, LoopBase *loop, unsigned time_out_sec)
    : FileDescriptor(fd), loop_(loop), time_out_sec_(time_out_sec) {
  auto flags = system_call(fcntl(fd, F_GETFL)).execption("fcntl");
  system_call(fcntl(fd, F_SETFL, flags | O_NONBLOCK)).execption("fcntl");
  loop_->poller()->register_fd(fd);
}

Task<Execpted<ssize_t>> AsyncFile::async_read(void *buf, size_t size) {
  while (true) {
    co_await waiting_for_event(loop_->poller(), fd(), PollEvent::read());
    auto result = system_call(::read(fd(), buf, size));
    if (result.is_nonblocking_error()) {
      continue;
    }
    co_return result;
  }
  // auto task = async_r<ssize_t>(
  //     [fd = this->fd(), buf, size]() {
  //       return detail::system_call(::read(fd, buf, size));
  //     },
  //     PollEvent::read());
  // auto task_handle = task.get_handle();
  // uint32_t timer_id = 0;
  // bool has_timer = false;
  // if (time_out_sec_ > 0) {
  //   has_timer = true;
  //   timer_id = loop_->timer()->add_timer(
  //       std::chrono::steady_clock::now() +
  //       std::chrono::seconds(time_out_sec_), [fd = this->fd(), loop =
  //       this->loop_, task_handle] {
  //         task_handle.promise().is_timeout = true;
  //         loop->poller()->remove_event(fd, PollEvent::read());
  //         task_handle.resume();
  //       });
  // }
  //
  // loop_->poller()->add_event(
  //     this->fd(), PollEvent::read(),
  //     [timer_id, has_timer, task_handle, loop = this->loop_] {
  //       if (has_timer) {
  //         loop->timer()->cancel_timer(timer_id);
  //       }
  //       task_handle.resume();
  //     });
  // return FinalAwaiter<detail::Execpted<ssize_t>>{std::move(task)};
}

Task<Execpted<ssize_t>> AsyncFile::async_read(ByteBuffer &buf) {
  return async_read(buf.data(), buf.size());
}

Task<Execpted<ssize_t>> AsyncFile::async_write(const void *buf, size_t size) {
  while (true) {
    co_await waiting_for_event(loop_->poller(), fd(), PollEvent::write());
    auto result = system_call(::write(fd(), buf, size));
    if (result.is_nonblocking_error()) {
      continue;
    }
    co_return result;
  }
}

Task<Execpted<ssize_t>> AsyncFile::async_write(std::string_view buf) {
  return async_write(buf.data(), buf.size());
}

Task<Execpted<int>> AsyncFile::async_accept(AddressSolver::Address &) {
  while (true) {
    co_await waiting_for_event(loop_->poller(), fd(), PollEvent::read());
    auto result = system_call(::accept(fd(), nullptr, nullptr));
    if (result.is_nonblocking_error()) {
      continue;
    }
    co_return result;
  }
}

Task<Execpted<int>>
AsyncFile::async_connect(AddressSolver::Address const &addr) {
  while (true) {
    co_await waiting_for_event(loop_->poller(), fd(), PollEvent::write());
    auto result = system_call(::connect(fd(), &addr.addr_, addr.len_));
    if (result.is_nonblocking_error()) {
      continue;
    }
    co_return result;
  }
}

AsyncFile AsyncFile::bind(AddressSolver::AddressInfo const &addr,
                          LoopBase *loop) {
  int fd = AsyncFile::create_listen(addr);
  auto async_file = AsyncFile(fd, loop);
  return async_file;
}

int AsyncFile::create_listen(AddressSolver::AddressInfo const &addr) {
  AddressSolver::Address serve = addr.get_address();
  int fd = addr.create_socket();
  int on = 1;
  system_call(setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)))
      .execption("setsockopt");
  system_call(setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &on, sizeof(on)))
      .execption("setsockopt");
  system_call(::bind(fd, &serve.addr_, serve.len_)).execption("bind");
  system_call(::listen(fd, SOMAXCONN)).execption("listen");
  return fd;
}

AsyncFile::~AsyncFile() {
  if (fd() != -1) {
    // std::cerr << "~AsyncFile() " << fd() << std::endl;
    loop_->poller()->unregister_fd(fd());
  }
}

} // namespace co_io
