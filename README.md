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

## TODO

1. HTTPS
2. Multithread

## Dependencies

1. C++ >= 20
2. [llhttp](https://github.com/nodejs/llhttp)

## Example

[Echo server](./example/echo_server.cpp)
[Http server](./example/http_server.cpp)

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
```

### Result

```
  Thread Stats   Avg      Stdev     Max   +/- Stdev
    Latency    39.94ms   99.67ms   1.99s    98.56%
    Req/Sec     3.76k   564.92     6.20k    76.14%
  296551 requests in 10.06s, 53.73MB read
Requests/sec:  29482.71
Transfer/sec:      5.34MB
```
