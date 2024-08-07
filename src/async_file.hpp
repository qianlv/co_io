#pragma once

#include <coroutine>
#include <cstring>
#include <fcntl.h>
#include <functional>
#include <memory>
#include <netdb.h>
#include <spdlog/spdlog.h>
#include <sys/socket.h>
#include <unistd.h>
#include <utility>

#include "poller.hpp"
#include "system_call.hpp"
#include "task.hpp"

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
      spdlog::debug("close {}", fd_);
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
public:
  explicit AsyncFile(int fd) : FileDescriptor(fd) {
    auto flags = detail::system_call(fcntl(fd, F_GETFL)).execption("fcntl");
    detail::system_call(fcntl(fd, F_SETFL, flags | O_NONBLOCK))
        .execption("fcntl");
    PollerBase::get().register_fd(fd);
  }

  template <typename Ret>
  Task<detail::system_call_value<Ret>> async_read(void *buf, size_t size) {
    int fd = this->fd();
    auto task = async_r<Ret>([fd, buf, size]() {
      spdlog::debug("async_read: {}", fd);
      return detail::system_call(::read(fd, buf, size));
    });
    PollerBase::get().add_event(fd, PollEvent::read(), task.get_handle());
    return std::move(task);
  }

  template <typename Ret>
  Task<detail::system_call_value<Ret>> async_write(void *buf, size_t size) {
    int fd = this->fd();
    auto task = async_r<ssize_t>([fd, buf, size]() {
      return detail::system_call(::write(fd, buf, size));
    });
    PollerBase::get().add_event(fd, PollEvent::write(), task.get_handle());
    return std::move(task);
  }

  template <typename Ret>
  Task<detail::system_call_value<Ret>> async_accept(AddressSolver::Address &) {
    int fd = this->fd();
    auto task = async_r<int>([fd]() {
      spdlog::debug("async_accept: {}", fd);
      return detail::system_call(::accept(fd, nullptr, nullptr));
    });
    PollerBase::get().add_event(fd, PollEvent::read(), task.get_handle());
    return std::move(task);
  }

  template <typename Ret>
  Task<detail::system_call_value<Ret>>
  async_connect(AddressSolver::Address const &addr) {
    int fd = this->fd();
    auto task = async_r<int>([fd, &addr]() {
      return detail::system_call(::connect(fd, &addr.addr_, addr.len_));
    });
    PollerBase::get().add_event(fd, PollEvent::write(), task.get_handle());
    return std::move(task);
  }

  static AsyncFile bind(AddressSolver::AddressInfo const &addr) {
    auto async_file = AsyncFile(addr.create_socket());
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
    spdlog::debug("desctructor: {}", fd());
    if (fd() != -1) {
      PollerBase::get().unregister_fd(fd());
    }
  }

private:
  template <typename T> struct OnceAwaiter {
    std::function<detail::system_call_value<T>()> func_;

    bool await_ready() const noexcept { return false; }
    void await_suspend(std::coroutine_handle<>) const noexcept {}
    detail::system_call_value<T> await_resume() const { return func_(); }
  };

  template <typename Ret>
  Task<detail::system_call_value<Ret>>
  async_r(std::function<detail::system_call_value<Ret>()> func) {
    auto onceAwaiter = OnceAwaiter<Ret>{std::move(func)};
    while (true) {
      auto ret = co_await onceAwaiter;
      // spdlog::debug("async_r: {} {} {}", fd(), ret.what(), ret.raw_value());
      if (!ret.is_nonblocking_error()) {
        PollerBase::get().remove_event(fd(), PollEvent::read_write());
        co_return ret;
      }
    }
  }
};
} // namespace co_io
