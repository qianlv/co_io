#pragma once

#include <coroutine>
#include <cstring>
#include <fcntl.h>
#include <functional>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>
#include <utility>

#include "poller.hpp"
#include "system_call.hpp"

namespace co_io {

class FileDescriptor {
public:
  explicit FileDescriptor(int fd) : fd_(fd) {}

  int fd() const { return fd_; }

  FileDescriptor(FileDescriptor &&other) : fd_(other.fd_) { other.fd_ = -1; }

  FileDescriptor &operator=(FileDescriptor &&other) {
    std::swap(fd_, other.fd_);
    return *this;
  }

  ~FileDescriptor() {
    if (fd_ != -1) {
      ::close(fd_);
    }
  }

private:
  int fd_ = -1;
};

struct AddressSolver {
  struct Address {
    union {
      struct sockaddr addr_;
      struct sockaddr_storage addr_storage_;
    };

    socklen_t len_ = sizeof(addr_storage_);
  };

  struct AddressInfo {
    struct addrinfo *ptr = nullptr;

    int create_socket() const {
      return detail::system_call(
                 ::socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol))
          .execption("socket");
    }

    Address get_address() const {
      Address ret{};
      std::memcpy(&ret.addr_storage_, ptr->ai_addr, ptr->ai_addrlen);
      ret.len_ = ptr->ai_addrlen;
      return ret;
    }

    [[nodiscard]] bool next() {
      if (ptr == nullptr) {
        return false;
      }
      ptr = ptr->ai_next;
      return ptr != nullptr;
    }
  };

  struct addrinfo *addrinfo_ = nullptr;

  AddressSolver(std::string_view host, std::string_view port) {
    int err = ::getaddrinfo(host.data(), port.data(), nullptr, &addrinfo_);
    if (err < 0) {
      throw std::runtime_error("getaddrinfo");
    }
  }

  AddressInfo get_address_info() { return {addrinfo_}; }

  AddressSolver() = default;
  AddressSolver(AddressSolver &&other) : addrinfo_(other.addrinfo_) {
    addrinfo_ = nullptr;
  }

  ~AddressSolver() {
    if (addrinfo_) {
      ::freeaddrinfo(addrinfo_);
    }
  }
};

class AsyncFile : public FileDescriptor {
  template <typename T> struct OnceAwaiter {
    std::function<detail::system_call_value<T>()> func_;

    bool await_ready() const noexcept { return false; }
    void await_suspend(std::coroutine_handle<>) const noexcept {}
    detail::system_call_value<T> await_resume() const { return func_(); }
  };
  template <typename T> struct TaskAwaiter {
    struct promise_type {
      std::exception_ptr exception_;
      T return_value_;
      using handle_type = std::coroutine_handle<promise_type>;
      std::coroutine_handle<> caller_;

      TaskAwaiter get_return_object() {
        return {handle_type::from_promise(*this)};
      }
      std::suspend_never initial_suspend() { return {}; }
      std::suspend_never final_suspend() noexcept { return {}; }
      void return_value(T value) noexcept {
        return_value_ = std::move(value);
        if (exception_) {
          std::rethrow_exception(exception_);
        }
        caller_.resume();
      }
      void unhandled_exception() { exception_ = std::current_exception(); }
    };

    auto get_handle() const { return handle_; }

    std::coroutine_handle<promise_type> handle_;
  };

  template <typename T> struct FinalAwaiter {
    TaskAwaiter<T> task;

    bool await_ready() const noexcept { return false; }
    void await_suspend(std::coroutine_handle<> h) const noexcept {
      task.handle_.promise().caller_ = h;
    }

    T await_resume() const { return task.get_handle().promise().return_value_; }
  };

public:
  explicit AsyncFile(int fd, std::shared_ptr<PollerBase> poller)
      : FileDescriptor(fd), poller_(std::move(poller)) {
    auto flags = detail::system_call(fcntl(fd, F_GETFL)).execption("fcntl");
    detail::system_call(fcntl(fd, F_SETFL, flags | O_NONBLOCK))
        .execption("fcntl");
    poller_->register_fd(fd);
  }

  FinalAwaiter<detail::system_call_value<ssize_t>> async_read(void *buf,
                                                              size_t size) {
    int fd = this->fd();
    auto task = async_r<ssize_t>([fd, buf, size]() {
      return detail::system_call(::read(fd, buf, size));
    });
    poller_->add_event(fd, PollEvent::read(), task.get_handle());
    return FinalAwaiter<detail::system_call_value<ssize_t>>{std::move(task)};
  }

  FinalAwaiter<detail::system_call_value<ssize_t>> async_write(void *buf,
                                                               size_t size) {
    int fd = this->fd();
    auto task = async_r<ssize_t>([fd, buf, size]() {
      return detail::system_call(::write(fd, buf, size));
    });
    poller_->add_event(fd, PollEvent::write(), task.get_handle());
    return FinalAwaiter<detail::system_call_value<ssize_t>>{std::move(task)};
  }

  FinalAwaiter<detail::system_call_value<int>>
  async_accept(AddressSolver::Address &) {
    int fd = this->fd();
    auto task = async_r<int>(
        [fd]() { return detail::system_call(::accept(fd, nullptr, nullptr)); });
    poller_->add_event(fd, PollEvent::read(), task.get_handle());
    return FinalAwaiter<detail::system_call_value<int>>{std::move(task)};
  }

  FinalAwaiter<detail::system_call_value<int>>
  async_connect(AddressSolver::Address const &addr) {
    int fd = this->fd();
    auto task = async_r<int>([fd, &addr]() {
      return detail::system_call(::connect(fd, &addr.addr_, addr.len_));
    });
    poller_->add_event(fd, PollEvent::write(), task.get_handle());
    return FinalAwaiter<detail::system_call_value<int>>{std::move(task)};
  }

  static AsyncFile bind(AddressSolver::AddressInfo const &addr,
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
    detail::system_call(::listen(async_file.fd(), SOMAXCONN))
        .execption("listen");
    return async_file;
  }

  AsyncFile(AsyncFile &&other) = default;
  AsyncFile &operator=(AsyncFile &&other) = default;

  AsyncFile(AsyncFile const &) = delete;
  AsyncFile &operator=(AsyncFile const &) = delete;

  ~AsyncFile() {
    if (fd() != -1) {
      poller_->unregister_fd(fd());
    }
  }

private:
  template <typename Ret>
  TaskAwaiter<detail::system_call_value<Ret>>
  async_r(std::function<detail::system_call_value<Ret>()> func) {
    auto onceAwaiter = OnceAwaiter<Ret>{std::move(func)};
    while (true) {
      auto ret = co_await onceAwaiter;
      if (!ret.is_nonblocking_error()) {
        poller_->remove_event(fd(), PollEvent::read_write());
        co_return ret;
      }
    }
  }

  std::shared_ptr<PollerBase> poller_;
};

} // namespace co_io
