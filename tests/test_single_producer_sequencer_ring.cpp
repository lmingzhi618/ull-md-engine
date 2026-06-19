#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>

#include "ull/core/single_producer_sequence.h"

int main() {
  constexpr std::uint64_t kCapacity = 4;
  {
    std::array<std::uint64_t, kCapacity> ring{};

    ull::SingleProducerSequencer sequencer(kCapacity);

    const auto s0 = sequencer.next();
    ring[s0 & (kCapacity - 1)] = 100;
    sequencer.publish(s0);

    sequencer.wait_until_available(0);
    assert(ring[0] == 100);
    sequencer.mark_consumed(0);

    const auto s1 = sequencer.next();
    ring[s1 & (kCapacity - 1)] = 200;
    sequencer.publish(s1);

    sequencer.wait_until_available(1);
    assert(ring[1] == 200);
    sequencer.mark_consumed(1);

    const auto s2 = sequencer.next();
    ring[s2 & (kCapacity - 1)] = 300;
    sequencer.publish(s2);

    sequencer.wait_until_available(2);
    assert(ring[2] == 300);
    sequencer.mark_consumed(2);

    const auto s3 = sequencer.next();
    ring[s3 & (kCapacity - 1)] = 400;
    sequencer.publish(s3);

    sequencer.wait_until_available(3);
    assert(ring[3] == 400);
    sequencer.mark_consumed(3);

    const auto s4 = sequencer.next();
    assert(s4 == 4);
    assert((s4 & (kCapacity - 1)) == 0);

    ring[s4 & (kCapacity - 1)] = 500;
    sequencer.publish(s4);

    sequencer.wait_until_available(4);
    assert(ring[0] == 500);
    sequencer.mark_consumed(4);
  }

  {
    ull::SingleProducerSequencer s(kCapacity);
    std::array<std::uint64_t, kCapacity> storage{};

    for (std::uint64_t i = 0; i < kCapacity; ++i) {
      const auto seq = s.next();
      storage[seq & (kCapacity - 1)] = 1000 + seq;
      s.publish(seq);
    }

    std::uint64_t seq{};
    assert(!s.try_next(seq));

    s.wait_until_available(0);
    assert(storage[0] == 1000);
    s.mark_consumed(0);

    assert(s.try_next(seq));
    assert(seq == kCapacity);
    storage[seq & (kCapacity - 1)] = 2000;

    assert(storage[0] == 2000);
  }
  std::cout << "test_single_producer_sequencer_ring PASS\n";
  return 0;
}
