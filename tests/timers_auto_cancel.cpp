#include <iostream>

#include "coroutine/task.hpp"
#include "coroutine/when_any.hpp"
#include "io/loop.hpp"
#include "io/poller.hpp"

using namespace co_io;

std::unique_ptr<LoopBase> loop;

Task<void> one_timer_complete(int n) {
    auto ret = co_await when_any(loop->timer()->sleep_for(std::chrono::seconds(n)),
                                 loop->timer()->sleep_for(std::chrono::seconds(n + 1)),
                                 loop->timer()->sleep_for(std::chrono::seconds(n + 2)),
                                 loop->timer()->sleep_for(std::chrono::seconds(n + 3)));
    std::cerr << "complete index " << ret.index << std::endl;
}

int main(int argc, char *argv[]) {
    EPollPoller poller;
    int n = 1;
    if (argc > 1) {
        n = atoi(argv[1]);
    }

    loop.reset(new EPollLoop());

    run_task(one_timer_complete(n));

    loop->run();

    return 0;
}
