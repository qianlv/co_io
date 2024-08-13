#include <iostream>

#include "async_file.hpp"
#include "loop.hpp"
#include "task.hpp"

using namespace co_io;

TaskNoSuspend<void> echo(std::string_view ip, std::string_view port,
                         std::shared_ptr<LoopBase> loop);
TaskNoSuspend<void> echo(std::string_view ip, std::string_view port,
                         std::shared_ptr<LoopBase> loop) {
  AsyncFile stdin_file{STDIN_FILENO, loop.get()};
  AddressSolver solver{ip, port};
  AddressSolver::AddressInfo info = solver.get_address_info();
  AsyncFile async_file{info.create_socket(), loop.get()};
  (co_await async_file.async_connect(info.get_address()))
      .execption("async_connect");

  std::array<char, 1024> buf;
  while (true) {
    auto ret = co_await stdin_file.async_read(buf.data(), buf.size());
    auto size = ret.value();
    if (size == 0) {
      break;
    }
    (co_await async_file.async_write(buf.data(), static_cast<size_t>(size)))
        .execption("async_write");
    size = (co_await async_file.async_read(buf.data(), buf.size()))
               .execption("async_read");
    (co_await stdin_file.async_write(buf.data(), static_cast<size_t>(size)))
        .execption("async_write");
  }
  std::cerr << "stop\n";
  loop->stop();
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

  if (debug) {
  }

  std::shared_ptr<LoopBase> loop;
  if (epoll) {
    loop.reset(new EPollLoop());
  } else {
    loop.reset(new SelectLoop());
  }

  auto t = echo(ip, port, loop);

  loop->run();
  return 0;
}
