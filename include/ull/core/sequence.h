#pragma once

#include <atomic>
#include <cstdint>

#include "ull/core/cacheline.h"

namespace ull {
static constexpr std::uint64_t kNoConsumerProgress =
    static_cast<std::uint64_t>(-1);

class alignas(kCacheLine) Sequence {
public:
  using value_type = std::uint64_t;
  explicit Sequence(std::uint64_t initial = 0) noexcept : value_(initial) {}

  std::uint64_t
  load(std::memory_order order = std::memory_order_acquire) const noexcept {
    return value_.load(order);
  }

  void store(std::uint64_t value,
             std::memory_order order = std::memory_order_release) noexcept {
    return value_.store(value, order);
  }

  std::uint64_t
  fetch_add(std::uint64_t delta,
            std::memory_order order = std::memory_order_relaxed) noexcept {
    return value_.fetch_add(delta, order);
  }

  bool compare_exchange_weak(
      std::uint64_t &expected, std::uint64_t desired,
      std::memory_order success = std::memory_order_relaxed,
      std::memory_order failure = std::memory_order_relaxed) noexcept {
    return value_.compare_exchange_weak(expected, desired, success, failure);
  }

private:
  std::atomic<std::uint64_t> value_;
};

static_assert(alignof(Sequence) == kCacheLine);
} // namespace ull
