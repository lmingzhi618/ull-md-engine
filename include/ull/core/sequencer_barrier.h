#include <cstdint>

#include "ull/core/single_producer_sequencer.h"
#include "ull/core/wait_strategy.h"

namespace ull {

class SequenceBarrier {
public:
  explicit SequenceBarrier(const SingleProducerSequencer *sequencer) noexcept
      : sequencer_(sequencer) {}

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
  }

private:
  const SingleProducerSequencer *sequencer_;
  BusySpinWaitStrategy wait_strategy_;
};
} // namespace ull
