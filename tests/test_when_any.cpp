#include "when_any.hpp"
#include "task.hpp"
#include <vector>

using namespace co_io;

Task<void> task1() {
  co_await std::suspend_always();
  std::cout << "start task1" << std::endl;
  co_return;
}

Task<void> task2() {
  co_await std::suspend_always();
  std::cout << "start task2" << std::endl;
  co_return;
}

Task<void> amain(Task<void> a1, Task<void> a2) {
  std::cout << "start when_any" << std::endl;
  co_await when_any(std::move(a1), std::move(a2));
  std::cerr << "amain done" << std::endl;
  co_return;
}

int main() { 
  auto a1 = task1();
  auto a2 = task2();
  auto h = a2.handle();
  auto a = amain(std::move(a1), std::move(a2));
  run_task(std::move(a));
  h.resume();
  std::cout << "main done" << std::endl;
  return 0;
}
