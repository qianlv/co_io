#pragma once

#include "async_file.hpp"
#include "http_connection.hpp"
#include "loop.hpp"
#include "task.hpp"
#include <memory>

namespace co_io {

template <typename LoopType> class HttpServer {
public:
  HttpServer(std::string_view ip, std::string port);

  void start() {
    accept();
    loop_->run();
  }

private:
  std::unique_ptr<LoopBase> loop_;
  AsyncFile listener_;

  TaskNoSuspend<void> accept();
  TaskNoSuspend<void> client(int fd);
};

template <typename LoopType>
HttpServer<LoopType>::HttpServer(std::string_view ip, std::string port)
    : loop_(std::make_unique<LoopType>()) {
  AddressSolver solver{ip, port};
  AddressSolver::AddressInfo info = solver.get_address_info();
  listener_ = AsyncFile::bind(info, loop_.get());
}

template <typename LoopType>
TaskNoSuspend<void> HttpServer<LoopType>::accept() {
  while (true) {
    AddressSolver::Address addr;
    int fd = co_await listener_.async_accept(addr);
    // std::cerr << "new connecton = " << fd << std::endl;
    client(fd);
  }
}

template <typename LoopType>
TaskNoSuspend<void> HttpServer<LoopType>::client(int fd) {
  HttpConnection conn(AsyncFile{fd, loop_.get(), 0});
  co_await conn.handle();
  co_return;
}

} // namespace co_io
