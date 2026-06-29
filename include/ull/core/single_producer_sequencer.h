#pragma once

#include <cstdint>

#include "ull/core/gating_sequences.h"
#include "ull/core/sequence.h"
#include "ull/core/utils.h"

namespace ull {

class SingleProducerSequencer {
public:
  explicit SingleProducerSequencer(std::uint64_t capacity) noexcept
      : capacity_(detail::validate_capacity(capacity)), next_(0),
        cursor_(kNoConsumerProgress),
        gating_(static_cast<std::uint64_t>(kNoConsumerProgress)),
        gating_sequences_(nullptr) {}

  bool try_next(std::uint64_t &out) noexcept {
    if (remaining_capacity() == 0) {
      return false;
    }

    out = next_;
    ++next_;
    return true;
  }

  std::uint64_t next() noexcept {
    std::uint64_t seq{};
    while (!try_next(seq)) {
      // spin until consumer advances the gating sequence
    }
    return seq;
  }

  std::uint64_t remaining_capacity() const noexcept {
    const auto gating = gating_min();
    const auto in_flight =
        (gating == kNoConsumerProgress) ? next_ : (next_ - gating - 1);
    return capacity_ - in_flight;
  }

  // Single-producer sequencer assumes in-order publication.
  // Publishing sequence N makes all sequences <= N visible.
  // Multi-producer or out-of-order publication requires per-slot
  // availability tracking instead of a single cursor.
  void publish(std::uint64_t seq) noexcept {
    cursor_.store(seq, std::memory_order_release);
  }

  std::uint64_t gating_min() const noexcept {
    if (gating_sequences_ != nullptr) {
      return gating_sequences_->load_min();
    }
    return gating_.load(std::memory_order_acquire);
  }

  void set_gating_sequences(GatingSequences *gating_sequences) noexcept {
    gating_sequences_ = gating_sequences;
  }
  void set_gating_sequence(std::uint64_t seq) noexcept {
    gating_.store(seq, std::memory_order_release);
  }

  void mark_consumed(std::uint64_t seq) noexcept { set_gating_sequence(seq); }

  std::uint64_t cursor() const noexcept {
    return cursor_.load(std::memory_order_acquire);
  }

  bool is_available(std::uint64_t seq) const noexcept {
    const auto current = cursor();
    if (current == kNoConsumerProgress) {
      return false;
    }
    return seq <= current;
  }

  // Convenience helper for early tests.
  // New consumer-side code should prefer SequenceBarrier so that
  // visibility waiting and wait strategy are kept outside the sequencer.
  void wait_until_available(std::uint64_t seq) const noexcept {
    while (!is_available(seq)) {
      // spin until producer publishes this sequence
    }
  }

  std::uint64_t gating_sequence() const noexcept {
    return gating_.load(std::memory_order_acquire);
  }

  std::uint64_t capacity() const noexcept { return capacity_; }

  std::uint64_t index(std::uint64_t seq) const noexcept {
    return seq & (capacity_ - 1);
  }

private:
  std::uint64_t capacity_;
  std::uint64_t next_;
  Sequence cursor_;
  Sequence gating_;
  GatingSequences *gating_sequences_;
};
} // namespace ull
