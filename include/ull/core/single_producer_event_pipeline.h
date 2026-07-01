#include "ull/core/sequence_barrier.h"
#include "ull/core/sequenced_ring.h"
#include "ull/core/single_producer_sequencer.h"

namespace ull {

// SingleProducerEventPipeline is a small composition layer.
//  It wires together:
// - SingleProducerSequencer for claim / publish / capacity
// - SequencedRing<T> for payload storage
// - SequenceBarrier for consumer-side visibility wait
//
// It is intentionally single-producder/single-consumer in v1.
// Fanout remains modeled by lower-level components for now.
template <typename T> class SingleProducerEventPipeline {
public:
  explicit SingleProducerEventPipeline(std::uint64_t capacity)
      : sequencer_(capacity), ring_(capacity), barrier_(&sequencer_) {}

  std::uint64_t next() noexcept { return sequencer_.next(); }

  bool try_next(std::uint64_t &seq) noexcept {
    return sequencer_.try_next(seq);
  }

  void publish(std::uint64_t seq) noexcept { sequencer_.publish(seq); }

  std::uint64_t wait_until_available(std::uint64_t seq) noexcept {
    return barrier_.wait_until_available(seq);
  }

  void write(std::uint64_t seq, const T &value) { ring_.write(seq, value); }

  T &read(std::uint64_t seq) noexcept { return ring_.read(seq); }

  const T &read(std::uint64_t seq) const noexcept { return ring_.read(seq); }

  void mark_consumed(std::uint64_t seq) noexcept {
    sequencer_.mark_consumed(seq);
  }

  std::uint64_t remaining_capacity() const noexcept {
    return sequencer_.remaining_capacity();
  }

private:
  SingleProducerSequencer sequencer_;
  SequencedRing<T> ring_;
  SequenceBarrier<> barrier_;
};
} // namespace ull
