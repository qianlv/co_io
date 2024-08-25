#include "task.hpp"
#include "when_all.hpp"

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
  std::cout << "start when_all" << std::endl;
  auto result = co_await when_all(std::move(a1), std::move(a2), std::move(a3));
  std::cerr << "when_all done" << std::endl;
  auto [t1, t2, t3] = result.value;
  std::cerr << "t1 = " << t1 << std::endl;
  std::cerr << "t2 = " << t2 << std::endl;
  std::cerr << "t3 = " << t3 << std::endl;
  std::cerr << "amain done" << std::endl;
  co_return;
}

int main() {
  auto a1 = task1();
  auto a2 = task2();
  auto a3 = task3();
  auto h1 = a1.handle();
  auto h2 = a2.handle();
  auto h3 = a3.handle();
  auto a = amain(std::move(a1), std::move(a2), std::move(a3));
  run_task(std::move(a));
  h1.resume();
  h2.resume();
  h3.resume();
  std::cout << "main done" << std::endl;
  return 0;
}
