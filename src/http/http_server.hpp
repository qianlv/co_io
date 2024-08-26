#pragma once

#include "coroutine/task.hpp"
#include "http/http_connection.hpp"
#include "http/http_router.hpp"
#include "io/async_file.hpp"
#include "io/loop.hpp"

#include <memory>
#include <thread>

namespace co_io {

template <typename LoopType> class HttpWorker {
public:
  HttpWorker(std::string_view ip, std::string_view port, HttpRouter &router,
             unsigned int time_out_sec)
      : loop_(std::make_unique<LoopType>()), router_(router),
        time_out_sec_(time_out_sec) {
    AddressSolver solver{ip, port};
    AddressSolver::AddressInfo info = solver.get_address_info();
    listener_ = AsyncFile::bind(info, loop_.get());
  }

  void start(bool new_thread) {
    if (new_thread) {
      th_ = std::jthread([this] {
        run_task(accept());
        loop_->run();
      });
    } else {
      run_task(accept());
      loop_->run();
    }
  }

  ~HttpWorker() {}

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

  Task<void> accept();
  Task<void> client(int fd);
};

template <typename LoopType> class HttpServer {
  using WorkerType = HttpWorker<LoopType>;
  using WorkerTypePointer = std::unique_ptr<WorkerType>;

public:
  HttpServer(std::string_view ip, std::string port);

  void start() {
    for (unsigned i = 0; i + 1 < nthreads_; ++i) {
      workers_.push_back(
          std::make_unique<WorkerType>(ip_, port_, router_, time_out_sec_));
    }
    for (auto &worker : workers_) {
      worker->start(true);
    }
    WorkerTypePointer worker =
        std::make_unique<WorkerType>(ip_, port_, router_, time_out_sec_);
    worker->start(false);
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
  std::string ip_, port_;
  HttpRouter router_;
  unsigned int time_out_sec_ = 0;
  unsigned int nthreads_ = 1;
  std::vector<WorkerTypePointer> workers_;
};

template <typename LoopType>
HttpServer<LoopType>::HttpServer(std::string_view ip, std::string port)
    : ip_(ip), port_(port) {}

template <typename LoopType> Task<void> HttpWorker<LoopType>::accept() {
  while (true) {
    AddressSolver::Address addr;
    auto ret = co_await listener_.async_accept(addr);
    if (ret.is_error()) {
    } else {
      std::cerr << "accept ok " << ret.value() << std::endl;
      run_task(client(ret.value()));
    }
  }
}

template <typename LoopType> Task<void> HttpWorker<LoopType>::client(int fd) {
  HttpConnection conn(AsyncFile{fd, loop_.get(), time_out_sec_}, router_);
  co_await conn.handle();
}

} // namespace co_io
