#pragma once

#include <cstdint>

#include "ull/core/sequence.h"

namespace ull {

class SingleProducerSequencer {
public:
  explicit SingleProducerSequencer(std::uint64_t initial = 0) noexcept
      : next_(initial), cursor_(initial) {}

  std::uint64_t next() noexcept {
    const auto seq = next_;
    ++next_;
    return seq;
  }

  void publish(std::uint64_t seq) noexcept {
    cursor_.store(seq, std::memory_order_release);
  }

  std::uint64_t cursor() const noexcept {
    return cursor_.load(std::memory_order_acquire);
  }

private:
  std::uint64_t next_;
  Sequence cursor_;
};
} // namespace ull
