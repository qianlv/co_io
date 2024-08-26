#include <iostream>

#include "coroutine/task.hpp"
#include "io/loop.hpp"
#include "io/poller.hpp"

using namespace co_io;

int count = 0;
int n = 1;

std::unique_ptr<LoopBase> loop;
Task<void> pause_echo(int i);

Task<void> pause_echo(int i) {
    co_await loop->timer()->sleep_for(std::chrono::seconds(i));
    std::cerr << "pause echo " << count << " " << i << std::endl;
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
        run_task(pause_echo(i));
    }

    run_task(loop->timer()->delay_run(std::chrono::seconds(n), []() {
        std::cerr << "delay run" << std::endl;
        run_task(pause_echo(1));
    }));

    loop->run();

    return 0;
}
