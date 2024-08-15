#pragma once

#include "async_file.hpp"
#include "http_connection.hpp"
#include "http_router.hpp"
#include "loop.hpp"
#include "task.hpp"
#include <memory>
#include <thread>

namespace co_io {

template <typename LoopType> class HttpWorker {
public:
  HttpWorker(int accept_fd, HttpRouter &router, unsigned int time_out_sec)
      : loop_(std::make_unique<LoopType>()), listener_(accept_fd, loop_.get()),
        router_(router), time_out_sec_(time_out_sec) {}

  void start(bool new_thread) {
    if (new_thread) {
      th_ = std::jthread([this] {
        accept();
        loop_->run();
      });
    } else {
      accept();
      loop_->run();
    }
  }

  ~HttpWorker() { std::cerr << "~HttpWorker()\n"; }

  HttpWorker(const HttpWorker &) = delete;
  HttpWorker &operator=(const HttpWorker &) = delete;

  HttpWorker(HttpWorker &&) = default;
  HttpWorker &operator=(HttpWorker &&) = default;

  using pointer_type = std::unique_ptr<HttpWorker<LoopType>>;

private:
  std::unique_ptr<LoopBase> loop_;
  AsyncFile listener_;
  HttpRouter &router_;
  unsigned int time_out_sec_ = 0;
  std::jthread th_;

  TaskNoSuspend<void> accept();
  TaskNoSuspend<void> client(int fd);
};

template <typename LoopType> class HttpServer {

public:
  HttpServer(std::string_view ip, std::string port);

  void start() {
    for (unsigned i = 0; i + 1 < nthreads_; ++i) {
      workers_.emplace_back(listener_fd_, router_, time_out_sec_);
    }
    for (auto &worker : workers_) {
      worker.start(true);
    }
    HttpWorker<LoopType> worker{listener_fd_, router_, time_out_sec_};
    worker.start(false);
  }

  HttpRouter &route() { return router_; }

  HttpServer &with_timeout(unsigned int time_out_sec) {
    this->time_out_sec_ = time_out_sec;
    return *this;
  }

  HttpServer &
  with_threads(unsigned int nthreads = std::thread::hardware_concurrency()) {
    this->nthreads_ = nthreads;
    return *this;
  }

private:
  int listener_fd_;
  HttpRouter router_;
  unsigned int time_out_sec_ = 0;
  unsigned int nthreads_ = 1;
  std::vector<HttpWorker<LoopType>> workers_;
};

template <typename LoopType>
HttpServer<LoopType>::HttpServer(std::string_view ip, std::string port) {
  AddressSolver solver{ip, port};
  AddressSolver::AddressInfo info = solver.get_address_info();
  listener_fd_ = AsyncFile::create_listen(info);
}

template <typename LoopType>
TaskNoSuspend<void> HttpWorker<LoopType>::accept() {
  while (true) {
    AddressSolver::Address addr;
    auto ret = co_await listener_.async_accept(addr);
    if (ret.is_error()) {
      std::cerr << "accept error " << ret.what() << std::endl;
    } else {
      client(ret.value());
    }
  }
}

template <typename LoopType>
TaskNoSuspend<void> HttpWorker<LoopType>::client(int fd) {
  HttpConnection conn(AsyncFile{fd, loop_.get(), time_out_sec_}, router_);
  co_await conn.handle();

  co_return;
}

} // namespace co_io
