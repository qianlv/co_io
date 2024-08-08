#include <iostream>
#include <spdlog/spdlog.h>

#include "loop.hpp"
#include "poller.hpp"
#include "task.hpp"

using namespace co_io;

int count = 0;
int n = 1;

std::unique_ptr<LoopBase> loop;
TaskNoSuspend<void> pause_echo(int i);

TaskNoSuspend<void> pause_echo(int i) {
  spdlog::info("pause start");
  co_await loop->timer()->sleep_for(std::chrono::seconds(i));
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

  loop.reset(new EPollLoop());

  for (int i = 0; i < n; i++) {
    pause_echo(i);
  }

  loop->timer()->delay_run(std::chrono::seconds(n), []() {
    spdlog::info("delay run");
    pause_echo(1);
  });

  loop->run();

  spdlog::debug("pause echo {} done", count);
  return 0;
}
