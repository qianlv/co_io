#pragma once

#include <coroutine>

template <typename Ts>
concept HasReturnType = requires {
    typename Ts::return_type;
};

template <class A>
concept Awaiter = requires(A a, std::coroutine_handle<> h) {
    {a.await_ready()};
    {a.await_suspend(h)};
    {a.await_resume()};
};

template <class A>
concept Awaitable = Awaiter<A> || requires(A a) {
    { a.operator co_await() } -> Awaiter;
} || requires(A a) {
    { operator co_await(a) } -> Awaiter;
};
