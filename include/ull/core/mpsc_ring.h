#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <type_traits>
#include <vector>

#include "ull/core/utils.h"

namespace ull {

template <class T> class MpscRing {
  static_assert(std::is_trivially_copyable_v<T>,
                "MpscRing<T> requires T to be trivially copyable");

public:
  explicit MpscRing(std::size_t capacity_pow2)
      : cap_(detail::validate_capacity(capacity_pow2)), mask_(cap_ - 1),
        buf_(cap_) {
    for (std::size_t i = 0; i < cap_; i++) {
      buf_[i].seq.store(i, std::memory_order_relaxed);
    }
  }

  bool push(const T &v) noexcept {
    const auto pos = head_.fetch_add(1, std::memory_order_relaxed);
    Cell &cell = buf_[pos & mask_];
    // Slot is reusable only when seq == pos.
    while (cell.seq.load(std::memory_order_acquire) != pos) {
      // v0.3 initial version: spin.
      // Later we can plug in SpinWait strategies.
    }

    cell.value = v;

    // Publish payload.
    cell.seq.store(pos + 1, std::memory_order_release);
    return true;
  }

  bool try_push(const T &v) noexcept {
    auto pos = head_.load(std::memory_order_relaxed);

    for (;;) {
      Cell &cell = buf_[pos & mask_];
      const auto seq = cell.seq.load(std::memory_order_acquire);

      if (seq != pos) {
        return false;
      }

      if (head_.compare_exchange_weak(pos, pos + 1, std::memory_order_relaxed,
                                      std::memory_order_relaxed)) {
        cell.value = v;
        cell.seq.store(pos + 1, std::memory_order_release);
        return true;
      }
      // CAS failed: another producer took this pos,
      // compare_exchange_weak updates pos to latest head value,
      // loop and check that new pos.
    }
  }

  bool try_pop(T &out) noexcept {
    Cell &cell = buf_[tail_ & mask_];

    // Ready only when producer has published seq == tail + 1.
    if (cell.seq.load(std::memory_order_acquire) != tail_ + 1) {
      return false;
    }

    out = cell.value;

    // Recycle slot for next ring round.
    cell.seq.store(tail_ + cap_, std::memory_order_release);
    ++tail_;
    return true;
  }

  std::size_t capacity() const noexcept { return cap_; }

private:
  struct Cell {
    std::atomic<std::uint64_t> seq{0};
    T value{};
  };

  std::size_t cap_;
  std::size_t mask_;
  std::vector<Cell> buf_;

  std::atomic<std::uint64_t> head_{0};

  // Single-consumer only: tail_ is private to consumer.
  std::uint64_t tail_{0};
};
} // namespace ull
