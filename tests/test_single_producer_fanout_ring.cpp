#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>

#include "ull/core/single_producer_sequencer.h"

int main() {
  {
    constexpr std::uint64_t kCapacity = 4;

    ull::SingleProducerSequencer sequencer(kCapacity);
    ull::GatingSequences gating(2);
    sequencer.set_gating_sequences(&gating);

    std::array<std::uint64_t, kCapacity> ring{};

    // Producer fills seq 0..3.
    for (std::uint64_t i = 0; i < kCapacity; ++i) {
      const auto seq = sequencer.next();
      ring[sequencer.index(seq)] = 100 + seq;
      sequencer.publish(seq);
    }

    assert(sequencer.remaining_capacity() == 0);

    // Consumer 0 reads everything.
    for (std::uint64_t seq = 0; seq <= 3; ++seq) {
      sequencer.wait_until_available(seq);
      assert(ring[sequencer.index(seq)] == 100 + seq);
      gating.mark_consumed(0, seq);
    }

    // Consumer 1 only reads seq 0.
    sequencer.wait_until_available(0);
    assert(ring[sequencer.index(0)] == 100);
    gating.mark_consumed(1, 0);

    assert(gating.load_min() == 0);
    assert(sequencer.remaining_capacity() == 1);

    std::uint64_t seq{};
    assert(sequencer.try_next(seq));
    assert(seq == 4);
    assert(sequencer.index(seq) == 0);

    ring[sequencer.index(seq)] = 200;
    sequencer.publish(seq);

    assert(sequencer.remaining_capacity() == 0);

    // Consumer1 catahes up to seq 2.
    for (std::uint64_t s = 1; s <= 2; ++s) {
      sequencer.wait_until_available(s);
      assert(ring[sequencer.index(s)] == 100 + s);
      gating.mark_consumed(1, s);
    }

    assert(gating.load_min() == 2);
    assert(sequencer.remaining_capacity() == 2);
  }

  std::cout << "test_single_producer_fanout_ring PASS\n";
  return 0;
}
