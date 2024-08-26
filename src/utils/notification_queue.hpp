#pragma once

#include <condition_variable>
#include <coroutine>
#include <mutex>
#include <queue>

namespace co_io {

template <typename T> class NotificationQueue {
  public:
    template <typename U>
    requires std::convertible_to<U, T>
    void push(U &&t) {
        {
            std::unique_lock<std::mutex> lock(mutex_);
            queue_.push(std::forward<U>(t));
        }
        ready_.notify_one();
    }

    template <typename U>
    requires std::convertible_to<U, T>
    bool try_push(U &&t) {
        {
            std::unique_lock<std::mutex> lock(mutex_, std::try_to_lock);
            if (!lock.owns_lock()) {
                return false;
            }
            queue_.push(std::forward<U>(t));
        }
        ready_.notify_one();
        return true;
    }

    bool pop(T &t) {
        std::unique_lock<std::mutex> lock(mutex_);
        ready_.wait(lock, [this] { return !queue_.empty() && !done_; });
        if (queue_.empty()) {
            return false;
        }
        t = std::move(queue_.front());
        queue_.pop();
        return true;
    }

    bool try_pop(T &t) {
        std::unique_lock<std::mutex> lock(mutex_, std::try_to_lock);
        if (!lock.owns_lock() || queue_.empty()) {
            return false;
        }
        t = std::move(queue_.front());
        queue_.pop();
        return true;
    }

    void done() {
        {
            std::unique_lock<std::mutex> lock(mutex_);
            done_ = true;
        }
        ready_.notify_all();
    }

  private:
    std::queue<T> queue_;
    std::mutex mutex_;
    std::condition_variable ready_;
    bool done_{false};
};

} // namespace co_io
