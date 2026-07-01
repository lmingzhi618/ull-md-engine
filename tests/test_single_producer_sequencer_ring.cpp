#include <cassert>
#include <cstdint>
#include <iostream>

#include "ull/core/sequence_barrier.h"
#include "ull/core/sequenced_ring.h"
#include "ull/core/single_producer_sequencer.h"

int main() {
  constexpr std::uint64_t kCapacity = 4;
  {
    ull::SequencedRing<std::uint64_t> ring(kCapacity);
    ull::SingleProducerSequencer sequencer(kCapacity);
    ull::SequenceBarrier barrier(&sequencer);

    const auto s0 = sequencer.next();
    ring.write(s0, 100);
    sequencer.publish(s0);

    barrier.wait_until_available(0);

    assert(ring.read(0) == 100);
    sequencer.mark_consumed(0);

    const auto s1 = sequencer.next();
    ring.write(s1, 200);
    sequencer.publish(s1);

    barrier.wait_until_available(1);
    assert(ring.read(1) == 200);
    sequencer.mark_consumed(1);

    const auto s2 = sequencer.next();
    ring.write(s2, 300);
    sequencer.publish(s2);

    barrier.wait_until_available(2);
    assert(ring.read(2) == 300);
    sequencer.mark_consumed(2);

    const auto s3 = sequencer.next();
    ring.write(s3, 400);
    sequencer.publish(s3);

    barrier.wait_until_available(3);
    assert(ring.read(3) == 400);
    sequencer.mark_consumed(3);

    const auto s4 = sequencer.next();
    assert(s4 == 4);
    assert(sequencer.index(s4) == 0);

    ring.write(s4, 500);
    sequencer.publish(s4);

    barrier.wait_until_available(4);
    assert(ring.read(0) == 500);
    sequencer.mark_consumed(4);
  }

  {
    ull::SingleProducerSequencer s(kCapacity);
    ull::SequenceBarrier barrier(&s);
    ull::SequencedRing<std::uint64_t> storage(kCapacity);

    for (std::uint64_t i = 0; i < kCapacity; ++i) {
      const auto seq = s.next();
      storage.write(seq, 1000 + seq);
      s.publish(seq);
    }

    std::uint64_t seq{};
    assert(!s.try_next(seq));

    barrier.wait_until_available(0);
    assert(storage.read(0) == 1000);
    s.mark_consumed(0);

    assert(s.try_next(seq));
    assert(seq == kCapacity);
    storage.write(seq, 2000);

    assert(storage.read(seq) == 2000);
    assert(storage.read(0) == 2000);
  }
  {
    ull::SingleProducerSequencer s(kCapacity);
    ull::SequenceBarrier barrier(&s);
    ull::GatingSequences g(2); // two consumers
    s.set_gating_sequences(&g);

    ull::SequencedRing<std::uint64_t> storage(kCapacity);

    for (std::uint64_t i = 0; i < kCapacity; ++i) {
      const auto seq = s.next();
      storage.write(seq, 100 + seq);
      s.publish(seq);
    }
    assert(s.remaining_capacity() == 0);

    g.mark_consumed(0, 3);
    g.mark_consumed(1, 0);

    assert(s.remaining_capacity() == 1);

    std::uint64_t seq{};
    assert(s.try_next(seq));
    assert(seq == 4);
    assert(!s.try_next(seq));

    storage.write(seq, 200);
    s.publish(seq);

    assert(storage.read(seq) == 200);
    assert(storage.read(0) == 200);
    assert(s.remaining_capacity() == 0);

    g.mark_consumed(1, 2);
    assert(s.remaining_capacity() == 2);
  }
  std::cout << "test_single_producer_sequencer_ring PASS\n";
  return 0;
}
