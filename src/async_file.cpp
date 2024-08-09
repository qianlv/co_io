#include "async_file.hpp"
#include "system_call.hpp"

namespace co_io {

AsyncFile::AsyncFile(int fd, std::shared_ptr<PollerBase> poller)
    : FileDescriptor(fd), poller_(std::move(poller)) {
  auto flags = detail::system_call(fcntl(fd, F_GETFL)).execption("fcntl");
  detail::system_call(fcntl(fd, F_SETFL, flags | O_NONBLOCK))
      .execption("fcntl");
  poller_->register_fd(fd);
}

AsyncFile::FinalAwaiter<detail::system_call_value<ssize_t>>
AsyncFile::async_read(void *buf, size_t size) {
  int fd = this->fd();
  auto task = async_r<ssize_t>(
      [fd, buf, size]() { return detail::system_call(::read(fd, buf, size)); });
  poller_->add_event(fd, PollEvent::read(), task.get_handle());
  return FinalAwaiter<detail::system_call_value<ssize_t>>{std::move(task)};
}

AsyncFile::FinalAwaiter<detail::system_call_value<ssize_t>>
AsyncFile::async_write(void *buf, size_t size) {
  int fd = this->fd();
  auto task = async_r<ssize_t>([fd, buf, size]() {
    return detail::system_call(::write(fd, buf, size));
  });
  poller_->add_event(fd, PollEvent::write(), task.get_handle());
  return FinalAwaiter<detail::system_call_value<ssize_t>>{std::move(task)};
}

AsyncFile::FinalAwaiter<detail::system_call_value<int>>
AsyncFile::async_accept(AddressSolver::Address &) {
  int fd = this->fd();
  auto task = async_r<int>(
      [fd]() { return detail::system_call(::accept(fd, nullptr, nullptr)); });
  poller_->add_event(fd, PollEvent::read(), task.get_handle());
  return FinalAwaiter<detail::system_call_value<int>>{std::move(task)};
}

AsyncFile::FinalAwaiter<detail::system_call_value<int>>
AsyncFile::async_connect(AddressSolver::Address const &addr) {
  int fd = this->fd();
  auto task = async_r<int>([fd, &addr]() {
    return detail::system_call(::connect(fd, &addr.addr_, addr.len_));
  });
  poller_->add_event(fd, PollEvent::write(), task.get_handle());
  return FinalAwaiter<detail::system_call_value<int>>{std::move(task)};
}

AsyncFile AsyncFile::bind(AddressSolver::AddressInfo const &addr,
                          PollerBasePtr poller) {
  auto async_file = AsyncFile(addr.create_socket(), std::move(poller));
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
AsyncFile::TaskAwaiter<detail::system_call_value<Ret>>
AsyncFile::async_r(std::function<detail::system_call_value<Ret>()> func) {
  auto onceAwaiter = OnceAwaiter<Ret>{std::move(func)};
  while (true) {
    auto ret = co_await onceAwaiter;
    if (!ret.is_nonblocking_error()) {
      poller_->remove_event(fd(), PollEvent::read_write());
      co_return ret;
    }
  }
}

AsyncFile::~AsyncFile() {
  if (fd() != -1) {
    poller_->unregister_fd(fd());
  }
}

} // namespace co_io
