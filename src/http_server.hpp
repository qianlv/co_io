#pragma once

#include "async_file.hpp"
#include "http_connection.hpp"
#include "loop.hpp"
#include "task.hpp"
#include <memory>

namespace co_io {

template <typename LoopType> class HttpServer {
public:
  HttpServer(std::string_view ip, std::string port,
             unsigned int time_out_sec = 0);

  void start() {
    accept();
    loop_->run();
  }

private:
  std::unique_ptr<LoopBase> loop_;
  AsyncFile listener_;
  unsigned int time_out_sec_ = 0;

  TaskNoSuspend<void> accept();
  TaskNoSuspend<void> client(int fd);
};

template <typename LoopType>
HttpServer<LoopType>::HttpServer(std::string_view ip, std::string port,
                                 unsigned int time_out_sec)
    : loop_(std::make_unique<LoopType>()), time_out_sec_(time_out_sec) {
  AddressSolver solver{ip, port};
  AddressSolver::AddressInfo info = solver.get_address_info();
  listener_ = AsyncFile::bind(info, loop_.get());
}

template <typename LoopType>
TaskNoSuspend<void> HttpServer<LoopType>::accept() {
  while (true) {
    AddressSolver::Address addr;
    int fd = co_await listener_.async_accept(addr);
    client(fd);
  }
}

template <typename LoopType>
TaskNoSuspend<void> HttpServer<LoopType>::client(int fd) {
  HttpConnection conn(AsyncFile{fd, loop_.get(), time_out_sec_});
  co_await conn.handle();
  co_return;
}

} // namespace co_io
