#pragma once

#include <coroutine>
#include <cstring>
#include <fcntl.h>
#include <functional>
#include <iostream>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>
#include <utility>

#include "byte_buffer.hpp"
#include "poller.hpp"
#include "system_call.hpp"

namespace co_io {

class FileDescriptor {
public:
  explicit FileDescriptor(int fd) : fd_(fd) {}
  FileDescriptor() = default;

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

class LoopBase;

class AsyncFile : public FileDescriptor {
public:
  template <typename ret_type> struct TaskAsnyc {
    struct promise_type;
    using handle_type = std::coroutine_handle<promise_type>;

    struct OnceAwaiter {
      std::function<ret_type()> func_;
      handle_type task_handle_;

      bool await_ready() const noexcept { return false; }
      void await_suspend(std::coroutine_handle<>) const noexcept {}
      ret_type await_resume() const {
        if (task_handle_.promise().is_timeout) {
          return ret_type{std::error_code(ECANCELED, std::system_category())};
        }
        return func_();
      }
    };

    struct promise_type {
      std::exception_ptr exception_;
      ret_type return_value_;
      std::coroutine_handle<> caller_;
      bool is_timeout = false;

      TaskAsnyc get_return_object() {
        return {handle_type::from_promise(*this)};
      }
      std::suspend_never initial_suspend() { return {}; }
      std::suspend_never final_suspend() noexcept { return {}; }

      OnceAwaiter await_transform(std::function<ret_type()> func) {
        return OnceAwaiter{func, handle_type::from_promise(*this)};
      }

      void return_value(ret_type value) noexcept {
        return_value_ = std::move(value);
        if (exception_) {
          std::rethrow_exception(exception_);
        }
        caller_.resume();
      }

      void unhandled_exception() {
        std::cerr << "unhandled exception" << std::endl;
        exception_ = std::current_exception();
      }

      ret_type result() {
        if (exception_) {
          std::rethrow_exception(exception_);
        }
        return return_value_;
      }
    };

    auto get_handle() const { return handle_; }

    handle_type handle_;
  };

  template <typename T> struct FinalAwaiter {
    TaskAsnyc<T> task;

    bool await_ready() const noexcept { return false; }
    void await_suspend(std::coroutine_handle<> h) const noexcept {
      task.handle_.promise().caller_ = h;
    }

    T await_resume() const { return task.get_handle().promise().result(); }
  };

  explicit AsyncFile(int fd, LoopBase *loop, unsigned time_out_sec = 0);

  FinalAwaiter<detail::Execpted<ssize_t>> async_read(void *buf, size_t size);
  FinalAwaiter<detail::Execpted<ssize_t>> async_read(ByteBuffer &buf);
  FinalAwaiter<detail::Execpted<ssize_t>> async_write(const void *buf,
                                                      size_t size);
  FinalAwaiter<detail::Execpted<ssize_t>> async_write(std::string_view buf);
  FinalAwaiter<detail::Execpted<int>> async_accept(AddressSolver::Address &);
  FinalAwaiter<detail::Execpted<int>>
  async_connect(AddressSolver::Address const &addr);
  static AsyncFile bind(AddressSolver::AddressInfo const &addr, LoopBase *loop);
  static int create_listen(AddressSolver::AddressInfo const &addr);

  AsyncFile() = default;
  AsyncFile(AsyncFile &&other) = default;
  AsyncFile &operator=(AsyncFile &&other) = default;

  AsyncFile(AsyncFile const &) = delete;
  AsyncFile &operator=(AsyncFile const &) = delete;

  ~AsyncFile();

private:
  template <typename Ret>
  TaskAsnyc<detail::Execpted<Ret>>
  async_r(std::function<detail::Execpted<Ret>()> func, PollEvent event);

  LoopBase *loop_ = nullptr;
  unsigned time_out_sec_ = 0;
};

} // namespace co_io
