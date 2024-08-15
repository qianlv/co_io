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

## TODO

1. HTTPS

## Dependencies

1. C++ >= 20
2. [llhttp](https://github.com/nodejs/llhttp)

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

./build/http_mt
wrk -t16 -c1000 http://127.0.0.1:12345/
```

### Result

```
Running 10s test @ http://127.0.0.1:12345/
  8 threads and 1000 connections
  Thread Stats   Avg      Stdev     Max   +/- Stdev
    Latency    30.51ms   26.57ms 991.59ms   99.25%
    Req/Sec     3.87k   639.68     5.45k    72.12%
  307879 requests in 10.09s, 55.79MB read
  Socket errors: connect 0, read 0, write 0, timeout 322
Requests/sec:  30524.07
Transfer/sec:      5.53MB

Running 10s test @ http://127.0.0.1:12345/
  16 threads and 1000 connections
  Thread Stats   Avg      Stdev     Max   +/- Stdev
    Latency     3.08ms    4.08ms  50.67ms   86.10%
    Req/Sec    34.61k     5.08k   68.98k    77.45%
  5551168 requests in 10.09s, 0.98GB read
Requests/sec: 550266.20
Transfer/sec:     99.71MB

```
