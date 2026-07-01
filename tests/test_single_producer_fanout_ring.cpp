#include <cassert>
#include <cstdint>
#include <iostream>

#include "ull/core/sequence_barrier.h"
#include "ull/core/sequenced_ring.h"
#include "ull/core/single_producer_sequencer.h"

int main() {
  {
    constexpr std::uint64_t kCapacity = 4;

    ull::SingleProducerSequencer sequencer(kCapacity);
    // Consumer-side visibility boundary.
    // Consumers wait on the barrier before reading ring storage.
    ull::SequenceBarrier barrier(&sequencer);
    ull::GatingSequences gating(2);
    sequencer.set_gating_sequences(&gating);

    ull::SequencedRing<std::uint64_t> ring(kCapacity);
    // Producer fills seq 0..3.
    for (std::uint64_t i = 0; i < kCapacity; ++i) {
      const auto seq = sequencer.next();
      ring.write(seq, 100 + seq);
      sequencer.publish(seq);
    }

    assert(sequencer.remaining_capacity() == 0);

    // Consumer 0 reads everything.
    for (std::uint64_t seq = 0; seq <= 3; ++seq) {
      // Visibility: wait until producer has published this sequence.
      barrier.wait_until_available(seq);
      assert(ring.read(seq) == 100 + seq);
      gating.mark_consumed(0, seq);
    }

    // Consumer 1 only reads seq 0.
    barrier.wait_until_available(0);
    assert(ring.read(0) == 100);
    gating.mark_consumed(1, 0);

    assert(gating.load_min() == 0);
    assert(sequencer.remaining_capacity() == 1);

    // Backpressure: consumer 1 is the slowest consumer at seq 0.
    // This releases exactly one slot, so producer can claim seq 4.
    std::uint64_t seq{};
    assert(sequencer.try_next(seq));
    assert(seq == 4);
    assert(sequencer.index(seq) == 0);

    // Slot reuse: seq 4 maps to physical slot 0.
    // This is safe only because all consumers have consumed seq 0.
    ring.write(seq, 200);
    sequencer.publish(seq);
    assert(ring.read(seq) == 200);
    assert(ring.read(0) == 200);
    assert(sequencer.remaining_capacity() == 0);

    // Consumer 1 can still catch up on seq 1 and seq 2 because their physical
    // slots have not been reused yet.
    for (std::uint64_t s = 1; s <= 2; ++s) {
      barrier.wait_until_available(s);
      assert(ring.read(s) == 100 + s);
      gating.mark_consumed(1, s);
    }

    assert(gating.load_min() == 2);
    assert(sequencer.remaining_capacity() == 2);
  }

  std::cout << "test_single_producer_fanout_ring PASS\n";
  return 0;
}
