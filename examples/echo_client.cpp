#include <iostream>
#include <spdlog/spdlog.h>

#include "async_file.hpp"
#include "poller.hpp"
#include "task.hpp"

using namespace co_io;

bool is_exit = false;

Task<void> echo(std::string_view ip, std::string_view port) {
  AsyncFile stdin_file{STDIN_FILENO};
  AddressSolver solver{ip, port};
  AddressSolver::AddressInfo info = solver.get_address_info();
  AsyncFile async_file{info.create_socket()};
  auto r = (co_await async_file.async_connect<int>(info.get_address()))
               .execption("async_connect");

  spdlog::debug("connected {}", r);
  std::array<char, 1024> buf;
  while (true) {
    auto size =
        (co_await stdin_file.async_read<ssize_t>(buf.data(), buf.size()))
            .value();
    if (size == 0) {
      spdlog::debug("Read EOF");
      break;
    }
    (co_await async_file.async_write<ssize_t>(buf.data(),
                                              static_cast<size_t>(size)))
        .execption("async_write");
    size = (co_await async_file.async_read<ssize_t>(buf.data(), buf.size()))
               .execption("async_read");
    (co_await stdin_file.async_write<ssize_t>(buf.data(),
                                              static_cast<size_t>(size)))
        .execption("async_write");
  }
  is_exit = true;
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
      std::cerr << "Usage: " << argv[0] << " [ip] [port] [-d] [-e]"
                << std::endl;
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

  echo(ip, port);

  while (!is_exit) {
    poller->poll();
  }
  return 0;
}
