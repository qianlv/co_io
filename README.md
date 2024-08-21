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
6. Adaptive Radix Tree

## TODO

1. HTTPS
2. Notification queue with Coroutine, Notification queue is suspend at the condition variable, it's conflict with Coroutine.
3. Adaptive Radix Tree remove operation
4. Router regex match with arguments

## Dependencies

- C++ >= 20
- [llhttp](https://github.com/nodejs/llhttp)
- [benchmark](https://github.com/google/benchmark)

## Example

[Echo server](./example/echo_server.cpp)
[Http server](./example/http.cpp)
[Multithread Http server](./example/http_mt.cpp)

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
