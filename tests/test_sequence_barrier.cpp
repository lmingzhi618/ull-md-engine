#include <cassert>
#include <cstdint>
#include <iostream>

#include "ull/core/sequence_barrier.h"
#include "ull/core/single_producer_sequencer.h"

int main() {
  {
    ull::SingleProducerSequencer sequencer(4);
    ull::SequenceBarrier barrier(&sequencer, nullptr);

    const auto seq = sequencer.next();

    // Before publish: consumer cannot see it.
    assert(!barrier.is_available(seq));

    // Publish: visibility boundary.
    sequencer.publish(seq);

    barrier.wait_until_available(seq);
    assert(barrier.is_available(seq));

    assert(!barrier.is_available(1));
  }
  {
    ull::SingleProducerSequencer sequencer(4);
    ull::BusySpinWaitStrategy strategy;
    ull::SequenceBarrier barrier(&sequencer, &strategy);

    const auto seq = sequencer.next();

    assert(!barrier.is_available(seq));

    sequencer.publish(seq);

    barrier.wait_until_available(seq);
    assert(barrier.is_available(seq));
  }
  std::cout << "test_sequence_barrier PASS\n";
  return 0;
}
