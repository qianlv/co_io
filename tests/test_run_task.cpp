#include "coroutine/task.hpp"

using namespace co_io;

Task<void> task(int id) {
  std::cout << "task " << id << std::endl;
  co_return;
}

int main() {
  run_task(task(1));
  return 0;
}
