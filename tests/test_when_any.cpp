#include "coroutine/task.hpp"
#include "coroutine/when_any.hpp"
#include <functional>

using namespace co_io;

Task<double> task1() {
    co_await std::suspend_always();
    std::cout << "start task1" << std::endl;
    co_return 1.11;
}

Task<std::string> task2() {
    co_await std::suspend_always();
    std::cout << "start task2" << std::endl;
    co_return "abc";
}

Task<int> task3() {
    co_await std::suspend_always();
    std::cout << "start task3" << std::endl;
    co_return 10000;
}

Task<void> amain(Task<double> a1, Task<std::string> a2, Task<int> a3) {
    std::cout << "start when_any" << std::endl;
    auto result = co_await when_any(std::move(a1), std::move(a2), std::move(a3));
    std::cerr << "when_any done" << std::endl;
    std::cout << "result.index = " << result.index << std::endl;
    if (std::holds_alternative<double>(result.value)) {
        std::cout << "value = " << std::get<double>(result.value) << std::endl;
    } else if (std::holds_alternative<std::string>(result.value)) {
        std::cout << "value = " << std::get<std::string>(result.value) << std::endl;
    } else if (std::holds_alternative<int>(result.value)) {
        std::cout << "value = " << std::get<int>(result.value) << std::endl;
    } else {
        std::cout << "unknown" << std::endl;
    }
    std::cerr << "amain done" << std::endl;
    co_return;
}

Task<int> task_id(int id) {
    co_await std::suspend_always();
    std::cerr << "task id " << id << " done" << std::endl;
    co_return id;
}

Task<void> amain2(int n, size_t index, std::coroutine_handle<> &h) {
    std::vector<Task<int>> tasks;
    for (int i = 0; i < n; i++) {
        tasks.push_back(task_id(i));
    }
    h = tasks[index].handle();
    auto result = co_await when_any(tasks);
    std::cout << "result.index = " << result.index << std::endl;
    std::cout << "result.value = " << result.value << std::endl;
    std::cerr << "amain2 done" << std::endl;
    co_return;
}

int main(int argc, const char *argv[]) {
    int n = 5;
    if (argc > 1) {
        n = atoi(argv[1]);
    }
    auto a1 = task1();
    auto a2 = task2();
    auto a3 = task3();
    auto h = a2.handle();
    auto a = amain(std::move(a1), std::move(a2), std::move(a3));
    run_task(std::move(a));
    h.resume();
    std::coroutine_handle<> h2;
    run_task(amain2(n, (size_t)n / 2, h2));
    h2.resume();
    std::cout << "main done" << std::endl;
    return 0;
}
