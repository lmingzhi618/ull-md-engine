#include <cassert>
#include <cstdint>
#include <iostream>

#include "ull/core/single_producer_sequence.h"

int main() {
  ull::SingleProducerSequencer seq;

  const auto a = seq.next();
  const auto b = seq.next();

  assert(a == 0);
  assert(b == 1);

  assert(seq.cursor() == 0);

  seq.publish(a);
  assert(seq.cursor() == 0);

  seq.publish(b);
  assert(seq.cursor() == 1);

  std::cout << "test_single_producer_sequencer PASS\n";

  return 0;
}
