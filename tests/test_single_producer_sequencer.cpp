#include <cassert>
#include <cstdint>
#include <iostream>

#include "ull/core/single_producer_sequence.h"

int main() {
  ull::SingleProducerSequencer s(4);

  assert(s.remaining_capacity() == 4);

  std::uint64_t seq{};

  assert(s.try_next(seq));
  assert(seq == 0);
  assert(s.remaining_capacity() == 3);

  assert(s.try_next(seq));
  assert(seq == 1);
  assert(s.remaining_capacity() == 2);

  assert(s.try_next(seq));
  assert(seq == 2);
  assert(s.remaining_capacity() == 1);

  assert(s.try_next(seq));
  assert(seq == 3);
  assert(s.remaining_capacity() == 0);

  // Capacity is full: consumer has not advanced yet.
  assert(!s.try_next(seq));
  assert(s.remaining_capacity() == 0);

  s.publish(3);
  assert(s.cursor() == 3);

  // Consumer has processed seq 0, so slot 0 is reusable.
  s.set_gating_sequence(0);
  assert(s.remaining_capacity() == 1);

  assert(s.try_next(seq));
  assert(seq == 4);
  assert(s.remaining_capacity() == 0);

  std::cout << "test_single_producer_sequencer PASS\n";

  return 0;
}
