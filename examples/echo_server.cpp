#include "async_file.hpp"
#include "loop.hpp"
#include "task.hpp"

#include <iostream>

using namespace co_io;

std::unique_ptr<LoopBase> loop;
TaskNoSuspend<void> client(AsyncFile async_file);
TaskNoSuspend<void> server(AsyncFile &async_file);

TaskNoSuspend<void> client(AsyncFile async_file) {
  char buffer[512];
  while (true) {
    auto t = co_await async_file.async_read(buffer, 512);
    // std::cerr << t.what() << std::endl;
    auto size = t.value();
    if (size == 0) {
      break;
    }
    co_await async_file.async_write(buffer, static_cast<size_t>(size));
  }
}

TaskNoSuspend<void> server(AsyncFile &async_file) {
  while (true) {
    AddressSolver::Address addr;
    auto t = co_await async_file.async_accept(addr);
    int fd = t.value();
    client(AsyncFile{fd, loop.get(), 0});
  }
}

int main(int argc, char *argv[]) {
  bool epoll = false;
  bool debug = false;
  size_t count = 0;
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
    } else if (argv[i] == std::string("-c") && i + 1 < argc) {
      count = static_cast<size_t>(atol(argv[++i]));
    } else if (i == 1) {
      ip = argv[i];
    } else if (i == 2) {
      port = argv[i];
    }
  }

  if (debug) {
  }

  if (epoll) {
    loop.reset(new EPollLoop(count));
  } else {
    loop.reset(new SelectLoop(count));
  }

  AddressSolver solver{"localhost", "12345"};
  AddressSolver::AddressInfo info = solver.get_address_info();
  AsyncFile async_file = AsyncFile::bind(info, loop.get());

  server(async_file);
  loop->run();
  return 0;
}
