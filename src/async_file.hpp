#pragma once

#include <unistd.h>
#include <fcntl.h>
#include <utility>

#include "system_call.hpp"

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

class AsyncFile: public FileDescriptor {
public:
  explicit AsyncFile(int fd) : FileDescriptor(fd) {
    auto flags = system_call(fcntl(fd, F_GETFL));
    system_call(fcntl(fd, F_SETFL, flags | O_NONBLOCK)).execption("fcntl");
  }

  AsyncFile(AsyncFile &&other) : FileDescriptor(std::move(other)) {}
  AsyncFile &operator=(AsyncFile &&other) {
    FileDescriptor::operator=(std::move(other));
    return *this;
  }
};
