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
    Latency    52.78ms   86.88ms   2.00s    98.95%
    Req/Sec     2.19k   641.97     4.19k    73.25%
  174730 requests in 10.05s, 31.66MB read
  Socket errors: connect 0, read 0, write 0, timeout 335
Requests/sec:  17384.99
Transfer/sec:      3.15MB

Running 10s test @ http://127.0.0.1:12345/
  16 threads and 1000 connections
  Thread Stats   Avg      Stdev     Max   +/- Stdev
    Latency     6.32ms    6.43ms 262.54ms   93.12%
    Req/Sec    10.96k     1.84k   34.32k    81.69%
  1753781 requests in 10.10s, 486.71MB read
Requests/sec: 173642.75
Transfer/sec:     48.19MB
```

### Reference

- [Adaptive Radix Tree](http://www-db.in.tum.de/~leis/papers/ART.pdf)
- [Radix Tree](https://en.wikipedia.org/wiki/Radix_tree)
