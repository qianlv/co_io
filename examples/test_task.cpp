#include "coroutine/task.hpp"

using namespace co_io;

Task<int> task_int() { co_return 1; }

Task<std::string> task_string() { co_return "hello"; }

Task<void> task_void() {
    std::cout << "Hello, world!" << std::endl;
    co_return;
}

Task<void> amain() {
    int ret_int = co_await task_int();
    std::cout << "task int: " << ret_int << std::endl;
    std::string str = co_await task_string();
    std::cout << "task string: " << str << std::endl;
    co_await task_void();
    std::cout << "Bye, world!" << std::endl;
}

int main() {
    run_task(amain());
    return 0;
}
