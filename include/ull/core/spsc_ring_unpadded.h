#pragma once
#include "ull/core/utils.h"
#include <atomic>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <type_traits>
#include <vector>

namespace ull {

// SPSC ring buffer without cache-line padding.: single producer, single
// This type exists only for false-sharing experiments.
// Semantics are intentionally identical to SpscRing<T>.
template <class T> class SpscRingUnpadded {
  static_assert(std::is_trivially_copyable_v<T>,
                "SpscRingUnpadded<T> requires T to be trivially copyable");

public:
  explicit SpscRingUnpadded(std::size_t capacity_pow2)
      : cap_(detail::validate_capacity(capacity_pow2)), mask_(cap_ - 1),
        buf_(cap_) {}

  bool try_push(const T &v) noexcept {
    const auto head = head_.load(std::memory_order_relaxed);
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

  // Intentionally unpadded: head_ and tail_ are placed adjacently
  // to demonstrate false sharing in benchmarks.
  std::atomic<std::uint64_t> head_{0};
  std::atomic<std::uint64_t> tail_{0};
};
} // namespace ull
