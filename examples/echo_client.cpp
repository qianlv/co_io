#include <iostream>
#include <spdlog/spdlog.h>

#include "async_file.hpp"
#include "poller.hpp"
#include "task.hpp"

using namespace co_io;

bool is_exit = false;

TaskVoid echo(std::string_view ip, std::string_view port) {
  AsyncFile stdin_file{STDIN_FILENO};
  AddressSolver solver{ip, port};
  AddressSolver::AddressInfo info = solver.get_address_info();
  AsyncFile async_file{info.create_socket()};
  auto r = (co_await async_file.async_connect<int>(info.get_address())).execption("async_connect");

  spdlog::debug("connected {}", r);
  std::array<char, 1024> buf;
  while (true) {
    auto size = (co_await stdin_file.async_read<ssize_t>(buf.data(), buf.size())).value();
    if (size == 0) {
      spdlog::debug("Read EOF");
      break;
    }
    (co_await async_file.async_write<ssize_t>(buf.data(), static_cast<size_t>(size))).execption("async_write");
    size = (co_await async_file.async_read<ssize_t>(buf.data(), buf.size())).execption("async_read");
    (co_await stdin_file.async_write<ssize_t>(buf.data(), static_cast<size_t>(size))).execption("async_write");
  }
  is_exit = true;
}

int main(int argc, char *argv[]) {
  if (argc != 3) {
    spdlog::error("Usage: {} ip port", argv[0]);
    return 1;
  }
  spdlog::set_level(spdlog::level::debug);

  SelectPoller poller;

  echo(argv[1], argv[2]);

  while (!is_exit) {
    poller.poll();
  }
  return 0;
}
