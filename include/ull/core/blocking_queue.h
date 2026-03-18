#pragma once

#include <condition_variable>
#include <cstddef>
#include <deque>
#include <mutex>
#include <stdexcept>

namespace ull {

template <class T> class BlockingQueue {
public:
  explicit BlockingQueue(std::size_t capacity) : capacity_(capacity) {
    if (capacity_ == 0) {
      throw std::invalid_argument(
          "BlockingQueue capacity must be greater than zero");
    }
  }

  BlockingQueue(const BlockingQueue &) = delete;
  BlockingQueue operator=(const BlockingQueue &) = delete;

  void push(const T &v) {
    std::unique_lock<std::mutex> lock(mu_);

    not_full_cv_.wait(lock, [&] {
      return closed_ || q_.size() < capacity_;
      ;
    });

    if (closed_) {
      throw std::runtime_error("push() on closed BlockingQueue");
    }

    q_.push_back(v);
    not_empty_cv_.notify_one();
  }

  bool pop(T &out) {
    std::unique_lock<std::mutex> lock(mu_);

    not_empty_cv_.wait(lock, [&] { return closed_ || !q_.empty(); });
    if (q_.empty()) {
      // If empty here, the queue must be closed.
      return false;
    }
    out = q_.front();
    q_.pop_front();
    not_full_cv_.notify_one();
    return true;
  }

  void close() {
    std::lock_guard<std::mutex> lock(mu_);
    closed_ = true;
    not_empty_cv_.notify_all();
    not_full_cv_.notify_all();
  }

  std::size_t capacity() const noexcept { return capacity_; }

private:
  std::size_t capacity_;
  std::deque<T> q_;
  bool closed_{false};

  mutable std::mutex mu_;
  std::condition_variable not_empty_cv_;
  std::condition_variable not_full_cv_;
};
} // namespace ull
