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

## Dependencies

1. C++ >= 20
2. [llhttp](https://github.com/nodejs/llhttp)

## Example
[Echo server](./example/echo_server.cpp)
[Http server](./example/http_server.cpp)

