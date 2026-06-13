#pragma once

#include <cstdint>

#include "ull/core/sequence.h"
#include "ull/core/utils.h"

namespace ull {

static constexpr std::uint64_t kNoConsumerProgress =
    static_cast<std::uint64_t>(-1);
class SingleProducerSequencer {
public:
  explicit SingleProducerSequencer(std::uint64_t capacity) noexcept
      : capacity_(detail::validate_capacity(capacity)), next_(-1), cursor_(0),
        gating_(static_cast<std::uint64_t>(-1)) {}

  bool try_next(std::uint64_t &out) noexcept {
    const auto gating = gating_.load(std::memory_order_acquire);
    const auto in_flight =
        (gating == kNoConsumerProgress) ? next_ : (next_ - capacity_ - 1);

    if (in_flight >= gating) {
      return false;
    }

    out = next_;
    ++next_;
    return true;
  }

  void publish(std::uint64_t seq) noexcept {
    cursor_.store(seq, std::memory_order_release);
  }

  void set_gating_sequence(std::uint64_t seq) noexcept {
    gating_.store(seq, std::memory_order_release);
  }

  std::uint64_t cursor() const noexcept {
    return cursor_.load(std::memory_order_acquire);
  }

  std::uint64_t gating_sequence() const noexcept {
    return gating_.load(std::memory_order_acquire);
  }

  std::uint64_t capacity() const noexcept { return capacity_; }

private:
  std::uint64_t capacity_;
  std::uint64_t next_;
  Sequence cursor_;
  Sequence gating_;
};
} // namespace ull
