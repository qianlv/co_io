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
    client(AsyncFile{fd, async_file.get_poller()});
  }
}

int main(int argc, char *argv[]) {
  (void)argc;
  (void)argv;

  spdlog::set_level(spdlog::level::debug);
  std::shared_ptr<PollerBase> poller = std::make_shared<SelectPoller>();
  // std::shared_ptr<PollerBase> poller = std::make_shared<EPollPoller>();

  AddressSolver solver{"localhost", "12345"};
  AddressSolver::AddressInfo info = solver.get_address_info();
  AsyncFile async_file = AsyncFile::bind(info, poller);

  server(async_file);
  while (true) {
    poller->poll();
  }
  return 0;
}
