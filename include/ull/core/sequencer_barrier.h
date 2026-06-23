#include <cstdint>

#include "ull/core/single_producer_sequencer.h"

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
      // Wait strategy v1: busy spin.
      // Later this can become yield / sleep / adaptive spin.
    }
  }

private:
  const SingleProducerSequencer *sequencer_;
};
} // namespace ull
