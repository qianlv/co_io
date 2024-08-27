# CO_IO

## Overview

Network libaray with c++20 coroutines

## Compiler

```
cmake -B build
cmake --build build
```

## Features

1. Coroutine
2. select/epoll event loop
3. io time out and timer, by timerfd with heap
4. HTTP 1.1
5. Multithread mode, using SO_REUSEADDR to dispatch fd when accept, [SO_REUSEADDR ref](https://lwn.net/Articles/542629/)
6. Adaptive Radix Tree to implement url router
7. Router regex match

## TODO

1. HTTPS
2. Notification queue with Coroutine, Notification queue is suspend at the condition variable, it's conflict with Coroutine.
3. Adaptive Radix Tree remove operation

## Usage samples

### co_io::Task<T = void> and co_io::run_task

- Task<T> is a coroutine task return a T value. 
- run_task is run task and auto destory all coroutine task when task return.

Example:

```cpp

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

```

### co_io::when_any

Waiting any one coroutine task done, and return the complete task result and index

Example:

```cpp
#include "coroutine/task.hpp"
#include "coroutine/when_any.hpp"

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
    auto result = co_await when_any(std::move(a1), std::move(a2), std::move(a3));
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
    std::cout << "amain done" << std::endl;
    co_return;
}

int main() {
    auto a1 = task1();
    auto a2 = task2();
    auto a3 = task3();
    auto h = a2.handle();
    auto a = amain(std::move(a1), std::move(a2), std::move(a3));
    run_task(std::move(a));
    h.resume();
    std::cout << "main done" << std::endl;
    return 0;
}
```

### co_io::when_all

Waiting all coroutine task done, and return a tuple of all complete task result,
for void using `VoidType` to place position.

Example:

```cpp
#include "coroutine/task.hpp"
#include "coroutine/when_all.hpp"

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
```

## Example

[Echo server](./example/echo_server.cpp)
[Http server](./example/http.cpp)
[Multithread Http server](./example/http_mt.cpp)

## Dependencies

- C++ >= 20
- [llhttp](https://github.com/nodejs/llhttp)
- [benchmark](https://github.com/google/benchmark)
- [re2](https://github.com/google/re2)

## Bench

### environment

OS: Ubuntu 22.04.4 LTS on Windows 10 x86_64
Kernel: 5.15.153.1-microsoft-standard-WSL2
CPU: AMD Ryzen 7 3700X (16) @ 3.600GHz
Memory: 16GB

### Tool

[wrk](https://github.com/wg/wrk)

### Command

```
./build/http
wrk -t8 -c1000 http://127.0.0.1:12345/

ulimit -n 4096
./build/http_mt
wrk -t16 -c1000 http://127.0.0.1:12345/
```

### Result

```
Running 10s test @ http://127.0.0.1:12345/
  8 threads and 1000 connections
  Thread Stats   Avg      Stdev     Max   +/- Stdev
    Latency    41.47ms  103.59ms   2.00s    98.52%
    Req/Sec     3.66k   642.85     5.64k    73.12%
  291508 requests in 10.10s, 52.82MB read
  Socket errors: connect 0, read 0, write 0, timeout 240
Requests/sec:  28871.07
Transfer/sec:      5.23MB

Running 10s test @ http://127.0.0.1:12345/
  16 threads and 1000 connections
  Thread Stats   Avg      Stdev     Max   +/- Stdev
    Latency     3.73ms    5.07ms 132.25ms   86.70%
    Req/Sec    26.77k     5.78k   50.01k    69.70%
  4286893 requests in 10.10s, 776.78MB read
Requests/sec: 424583.96
Transfer/sec:     76.93MB
```

### Reference

- [Adaptive Radix Tree](http://www-db.in.tum.de/~leis/papers/ART.pdf)
- [Radix Tree](https://en.wikipedia.org/wiki/Radix_tree)
