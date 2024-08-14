#include <iostream>

#include "loop.hpp"
#include "poller.hpp"
#include "task.hpp"

using namespace co_io;

int count = 0;
int n = 1;

std::unique_ptr<LoopBase> loop;
TaskNoSuspend<void> pause_echo(int i);

TaskNoSuspend<void> pause_echo(int i) {
  co_await loop->timer()->sleep_for(std::chrono::seconds(i));
  count += 1;
}

int main(int argc, char *argv[]) {
  EPollPoller poller;
  n = 1;
  if (argc > 1) {
    n = atoi(argv[1]);
  }

  loop.reset(new EPollLoop());

  for (int i = 0; i < n; i++) {
    pause_echo(i);
  }

  loop->timer()->delay_run(std::chrono::seconds(n), []() {
    std::cerr << "delay run" << std::endl;
    pause_echo(1);
  });

  loop->run();

  return 0;
}
