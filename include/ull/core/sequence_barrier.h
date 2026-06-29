#pragma once

#include <cstdint>

#include "ull/core/single_producer_sequencer.h"
#include "ull/core/wait_strategy.h"

namespace ull {

template <typename WaitStrategy = BusySpinWaitStrategy> class SequenceBarrier {
public:
  explicit SequenceBarrier(const SingleProducerSequencer *sequencer) noexcept
      : sequencer_(sequencer), wait_strategy_() {}

  explicit SequenceBarrier(const SingleProducerSequencer *sequencer,
                           WaitStrategy wait_strategy) noexcept
      : sequencer_(sequencer), wait_strategy_(wait_strategy) {}

  bool is_available(std::uint64_t seq) const noexcept {
    return sequencer_ != nullptr && sequencer_->is_available(seq);
  }

  std::uint64_t wait_until_available(std::uint64_t seq) noexcept {
    while (!is_available(seq)) {
      if (sequencer_ == nullptr) {
        return kNoConsumerProgress;
      }
      // Wait: consumer-side visibility wait.
      // BusySpinWaitStrategy keeps the thread active to minimize wakeup
      // latency.
      wait_strategy_.idle();
    }
    wait_strategy_.reset();
    return sequencer_->cursor();
  }

private:
  const SingleProducerSequencer *sequencer_;
  WaitStrategy wait_strategy_;
};
} // namespace ull
