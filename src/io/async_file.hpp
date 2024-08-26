#pragma once

#include <cstring>
#include <fcntl.h>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>
#include <utility>

#include "utils/byte_buffer.hpp"
#include "utils/system_call.hpp"
#include "coroutine/task.hpp"

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
      return system_call(
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
  explicit AsyncFile(int fd, LoopBase *loop, unsigned time_out_sec = 0);

  Task<Execpted<ssize_t>> async_read(void *buf, size_t size);
  Task<Execpted<ssize_t>> async_read(ByteBuffer &buf);
  Task<Execpted<ssize_t>> async_write(const void *buf, size_t size);
  Task<Execpted<ssize_t>> async_write(std::string_view buf);
  Task<Execpted<int>> async_accept(AddressSolver::Address &);
  Task<Execpted<int>> async_connect(AddressSolver::Address const &addr);
  static AsyncFile bind(AddressSolver::AddressInfo const &addr, LoopBase *loop);
  static int create_listen(AddressSolver::AddressInfo const &addr);

  AsyncFile() = default;
  AsyncFile(AsyncFile &&other) = default;
  AsyncFile &operator=(AsyncFile &&other) = default;

  AsyncFile(AsyncFile const &) = delete;
  AsyncFile &operator=(AsyncFile const &) = delete;

  ~AsyncFile();

private:
  LoopBase *loop_ = nullptr;
  unsigned time_out_sec_ = 0;
};

} // namespace co_io
