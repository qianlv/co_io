#pragma once

#include <array>
#include <functional>
#include <memory>
#include <sys/epoll.h>
#include <sys/select.h>
#include <unordered_map>

#include "coroutine/task.hpp"

namespace co_io {

class PollEvent {
  private:
    unsigned int event;
    constexpr PollEvent(unsigned int ev) : event(ev) {}
    constexpr PollEvent() : event(0) {}

  public:
    constexpr PollEvent operator&(PollEvent rhs) const { return event & rhs.event; }

    constexpr PollEvent operator|(PollEvent rhs) const { return PollEvent(event | rhs.event); }

    constexpr PollEvent operator^(PollEvent rhs) const { return PollEvent(event ^ rhs.event); }

    constexpr operator bool() const noexcept { return event != 0; }

    bool operator==(PollEvent rhs) const { return event == rhs.event; }

    constexpr PollEvent operator~() const { return PollEvent(~event); }

    static constexpr PollEvent none() { return PollEvent(0); }
    static constexpr PollEvent read() { return PollEvent(1 << 0); }
    static constexpr PollEvent write() { return PollEvent(1 << 1); }
    static constexpr PollEvent read_write() { return read() | write(); }

    constexpr unsigned int raw() const { return event; }
};

class PollerBase {
  public:
    using callback = std::function<void()>;
    struct PollerEvent {
        int fd;
        PollEvent event;
        callback read_handle = {};
        callback write_handle = {};
    };

    virtual void register_fd(int fd);
    virtual void unregister_fd(int fd);
    virtual void add_event(int fd, PollEvent event, callback handle);
    virtual bool remove_event(int fd, PollEvent event);
    virtual void poll() = 0;

    virtual ~PollerBase() = default;

  protected:
    std::unordered_map<int, PollerEvent> handles_;
};

class SelectPoller : public PollerBase {
  public:
    SelectPoller();

    void register_fd(int) override;
    void unregister_fd(int fd) override;
    void add_event(int fd, PollEvent event, callback handle) override;
    bool remove_event(int fd, PollEvent event) override;
    void poll() override;

  private:
    fd_set read_set_;
    fd_set write_set_;
    int max_fd_ = 0;
};

class EPollPoller : public PollerBase {
  public:
    EPollPoller();
    ~EPollPoller() override;

    void register_fd(int fd) override;
    void unregister_fd(int fd) override;
    void add_event(int fd, PollEvent event, callback handle) override;
    bool remove_event(int fd, PollEvent event) override;
    void poll() override;

  private:
    int epoll_fd_;
    std::array<struct epoll_event, 1024> events_{};
};

using PollerBasePtr = std::unique_ptr<PollerBase>;

struct PollerPromise : public Promise<void> {
    int fd_;
    PollerBase *poller_;
    PollEvent event_ = PollEvent::read();

    auto get_return_object() { return std::coroutine_handle<PollerPromise>::from_promise(*this); }
    inline ~PollerPromise() { poller_->remove_event(fd_, event_); }

    PollerPromise &operator=(PollerPromise &&) = delete;
};

struct PollerAwaiter {
    int fd_;
    PollerBase *poller_;
    PollEvent event_ = PollEvent::read();

    bool await_ready() const noexcept { return false; }
    void await_suspend(std::coroutine_handle<PollerPromise> h) const {
        poller_->add_event(fd_, event_, h);
        h.promise().poller_ = poller_;
        h.promise().fd_ = fd_;
        h.promise().event_ = event_;
    }
    void await_resume() const noexcept {}
};

inline Task<void, PollerPromise> waiting_for_event(PollerBase *poller, int fd, PollEvent event) {
    co_await PollerAwaiter{fd, poller, event};
}

} // namespace co_io
