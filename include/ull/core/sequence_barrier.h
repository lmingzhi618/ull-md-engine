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
                           const WaitStrategy wait_strategy) noexcept
      : sequencer_(sequencer), wait_strategy_(wait_strategy) {}

  bool is_available(std::uint64_t seq) const noexcept {
    return sequencer_ != nullptr && sequencer_->is_available(seq);
  }

  void wait_until_available(std::uint64_t seq) const noexcept {
    while (!is_available(seq)) {
      // Wait: consumer-side visibility wait.
      // BusySpinWaitStrategy keeps the thread active to minimize wakeup
      // latency.
      wait_strategy_.idle();
    }
    wait_strategy_.reset();
  }

private:
  const SingleProducerSequencer *sequencer_;
  const WaitStrategy wait_strategy_;
};
} // namespace ull
