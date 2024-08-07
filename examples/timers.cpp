#include <iostream>
#include <spdlog/spdlog.h>

#include "poller.hpp"
#include "timer_util.hpp"

using namespace co_io;

int count = 0;
int n = 1;

Task<void> pause_echo(int i) {
  spdlog::info("pause start");
  co_await sleep_for(std::chrono::seconds(i));
  spdlog::info("pause end");
  count += 1;
}


int main(int argc, char *argv[]) {
  EPollPoller poller;
  n = 1;
  if (argc > 1) {
    n = atoi(argv[1]);
  }

  spdlog::set_level(spdlog::level::debug);

  std::vector<Task<void>> tasks;
  for (int i = 0; i < n; i++) {
    tasks.push_back(pause_echo(i + 1));
  }

  while (count < n) {
    poller.poll();
  }

  spdlog::debug("pause echo {} done", count);
  return 0;
}
