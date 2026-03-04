#pragma once
#include "ull/core/cacheline.h"
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <vector>

namespace ull {

// SPSC ring buffer: single producer, single consumer.
// Only supports trivially copyable T for MVP simplicity and speed.
template <class T> class SpscRing {
  static_assert(std::is_trivially_copyable_v<T>,
                "T must be trivially copyable");

public:
  explicit SpscRing(std::size_t capacity_pow2)
      : cap_(capacity_pow2), mask_(cap_ - 1), buf_(cap_) {
    // MVP: assume caller passes power-of-2. (We'll add validation later.)
  }

  bool try_push(const T &v) noexcept {
    const auto head =
        head_.load(std::memory_order_relaxed); // ? Where here is relaxed, while
                                               // tail_load is acquire
    const auto next = head + 1;

    // If next - tail > cap, buffer is full.
    if (next - tail_.load(std::memory_order_acquire) > cap_)
      return false;

    buf_[head & mask_] = v;                       // write payload first
    head_.store(next, std::memory_order_release); // publish
    return true;
  }

  bool try_pop(T &out) noexcept {
    const auto tail = tail_.load(std::memory_order_relaxed);

    // If head == tail, buffer is empty.
    if (head_.load(std::memory_order_acquire) == tail)
      return false;

    out = buf_[tail & mask_]; // read payload after acquire
    tail_.store(tail + 1, std::memory_order_release);
    return true;
  }

  std::size_t capacity() const noexcept { return cap_; }

private:
  std::size_t cap_;
  std::size_t mask_;
  std::vector<T> buf_;

  // Put head and tail on separate cache lines to reduce false sharing.
  alignas(kCacheLine) std::atomic<std::uint64_t> head_{0};
  alignas(kCacheLine) std::atomic<std::uint64_t> tail_{0};
};
} // namespace ull
