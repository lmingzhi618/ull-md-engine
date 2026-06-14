#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>

#include "ull/core/single_producer_sequence.h"

int main() {
  constexpr std::uint64_t kCapacity = 4;
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

  std::cout << "test_single_producer_sequencer_ring PASS\n";
  return 0;
}
