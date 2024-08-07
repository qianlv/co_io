#include <iostream>
#include <spdlog/spdlog.h>

#include "async_file.hpp"
#include "poller.hpp"
#include "task.hpp"

using namespace co_io;

TaskVoid client(AsyncFile async_file) {
  char buffer[1024];
  while (true) {
    spdlog::debug("waiting for read");
    auto t = co_await async_file.async_read<ssize_t>(buffer, 1024);
    auto size = t.value();
    if (size == 0) {
      spdlog::debug("Read EOF");
      break;
    }
    spdlog::debug("Read: {} bytes", size);
    co_await async_file.async_write<ssize_t>(buffer, static_cast<size_t>(size));
  }
  spdlog::debug("client exit");
}

TaskVoid server(AsyncFile &async_file) {
  while (true) {
    AddressSolver::Address addr;
    spdlog::debug("waiting for accept");
    auto t = co_await async_file.async_accept<int>(addr);
    int fd = t.value();
    spdlog::debug("new connection: {}", fd);
    client(AsyncFile{fd});
  }
}

int main(int argc, char *argv[]) {
  bool epoll = false;
  bool debug = false;
  std::string ip = "localhost";
  std::string port = "12345";
  for (int i = 1; i < argc; ++i) {
    if (argv[i] == std::string("-e")) {
      epoll = true;
    } else if (argv[i] == std::string("-d")) {
      debug = true;
    } else if (argv[i] == std::string("-h")) {
      std::cerr << "Usage: " << argv[0] << " [ip] [port] [-d] [-e]" << std::endl;
    } else if (i == 1) {
      ip = argv[i];
    } else if (i == 2) {
      port = argv[i];
    }
  }
  spdlog::info("ip: {}, port: {}", ip, port);

  if (debug) {
    spdlog::set_level(spdlog::level::debug);
  }

  std::unique_ptr<PollerBase> poller;
  if (epoll) {
    poller.reset(new EPollPoller());
  } else {
    poller.reset(new SelectPoller());
  }

  AddressSolver solver{"localhost", "12345"};
  AddressSolver::AddressInfo info = solver.get_address_info();
  AsyncFile async_file = AsyncFile::bind(info);

  server(async_file);
  while (true) {
    poller->poll();
  }
  return 0;
}
