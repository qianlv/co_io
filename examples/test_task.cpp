#include "coroutine/task.hpp"
#include "io/async_file.hpp"
#include "io/loop.hpp"
#include "io/poller.hpp"

using namespace co_io;

Task<int> task2() { co_return 2; }

Task<int> task1() {
    int r = co_await task2();
    std::cerr << "r = " << r << std::endl;
    co_return 1;
}
int main() {
    std::cerr << "main" << std::endl;
    auto t = task1();
    t.get_handle().resume();
    std::cerr << "main2" << std::endl;
    return 0;
}
