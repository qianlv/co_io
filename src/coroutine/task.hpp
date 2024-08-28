#pragma once
#include <cassert>
#include <coroutine>
#include <exception>
#include <iostream>
#include <utility>

#include "utils/uninitialized.hpp"

namespace co_io {

template <typename P = void> struct Self {
    bool await_ready() { return false; }

    bool await_suspend(std::coroutine_handle<P> h) {
        H = h;
        return false;
    }

    auto await_resume() noexcept { return H; }

    std::coroutine_handle<P> H;
};

template <> struct Self<void> {
    bool await_ready() { return false; }

    bool await_suspend(std::coroutine_handle<> h) {
        H = h;
        return false;
    }

    auto await_resume() noexcept { return H; }

    std::coroutine_handle<> H;
};

struct PreviousAwaiter {
    std::coroutine_handle<> previous_handle_;

    bool await_ready() const noexcept { return false; }

    std::coroutine_handle<> await_suspend(std::coroutine_handle<>) const noexcept {
        return previous_handle_;
    }

    void await_resume() const noexcept {}
};

template <typename T = void> struct Promise {
    std::coroutine_handle<> previous_handle_{};
    std::exception_ptr exception_{};
    Uninitialized<T> result_;

    auto get_return_object() { return std::coroutine_handle<Promise>::from_promise(*this); }

    std::suspend_always initial_suspend() { return {}; }
    auto final_suspend() noexcept { return PreviousAwaiter{previous_handle_}; }
    auto return_value(T &&value) { result_ = std::move(value); }
    auto return_value(const T &value) noexcept { result_ = value; }
    void unhandled_exception() { exception_ = std::current_exception(); }

    T result() {
        if (exception_) [[unlikely]] {
            std::rethrow_exception(exception_);
        }
        return result_.move();
    }

    Promise &operator=(Promise &&) = delete;
};

template <> struct Promise<void> {
    std::coroutine_handle<> previous_handle_{};
    std::exception_ptr result_{};

    auto get_return_object() { return std::coroutine_handle<Promise>::from_promise(*this); }

    std::suspend_always initial_suspend() { return {}; }
    auto final_suspend() noexcept {
        assert(previous_handle_);
        return PreviousAwaiter{previous_handle_};
    }
    auto return_void() noexcept { result_ = nullptr; }
    void unhandled_exception() { result_ = std::current_exception(); }

    void result() {
        if (result_) {
            std::rethrow_exception(result_);
        }
    }

    Promise &operator=(Promise &&) = delete;
};

template <typename T, typename P = Promise<T>> class Task {
  public:
    using promise_type = P;
    using return_type = T;

    Task(std::coroutine_handle<promise_type> handle) noexcept : handle_(handle) {
        // std::cerr << "Task " << handle.address() << std::endl;
    }
    Task(Task &&other) noexcept : handle_(other.handle_) { other.handle_ = nullptr; }
    Task &operator=(Task &&other) noexcept {
        using std::swap;
        swap(handle_, other.handle_);
        return *this;
    }
    Task(const Task &) = delete;
    Task &operator=(const Task &) = delete;

    ~Task() {
        if (handle_) {
            // std::cerr << "~Task " << handle_.address() << std::endl;
            handle_.destroy();
        } else {
            // std::cerr << "~Task nullptr" << std::endl;
        }
    }

    struct Awaiter {
        std::coroutine_handle<promise_type> handle_;

        bool await_ready() const noexcept { return false; }

        std::coroutine_handle<promise_type>
        await_suspend(std::coroutine_handle<> h) const noexcept {
            handle_.promise().previous_handle_ = h;
            return handle_;
        }

        T await_resume() const { return handle_.promise().result(); }
    };

    Awaiter operator co_await() const noexcept { return {handle_}; }

    std::coroutine_handle<promise_type> handle() const noexcept { return handle_; }

    operator std::coroutine_handle<promise_type>() const noexcept { return handle_; }

    std::coroutine_handle<promise_type> release() noexcept {
        return std::exchange(handle_, nullptr);
    }

  private:
    std::coroutine_handle<promise_type> handle_;
};

struct AutoDestoryPromise {
    struct AutoDestoryAwaiter {
        bool await_ready() const noexcept { return false; }
        void await_suspend(std::coroutine_handle<> h) const noexcept {
            // std::cerr << "AutoDestoryAwaiter " << h.address() << std::endl;
            h.destroy();
        }
        void await_resume() const noexcept {}
    };

    auto get_return_object() {
        return std::coroutine_handle<AutoDestoryPromise>::from_promise(*this);
    }

    std::suspend_always initial_suspend() { return {}; }
    auto final_suspend() noexcept { return AutoDestoryAwaiter{}; }
    auto return_void() noexcept {}
    void unhandled_exception() { std::terminate(); }

    void result() {}

    AutoDestoryPromise &operator=(AutoDestoryPromise &&) = delete;
};

template <typename T> Task<void, AutoDestoryPromise> auto_destory(Task<T> task) {
    (void)co_await task;
}

template <typename T, typename P = Promise<T>> auto run_task(Task<T, P> task) {
    auto wrapper = auto_destory(std::move(task));
    auto handle = wrapper.release();
    handle.resume();
}

} // namespace co_io
