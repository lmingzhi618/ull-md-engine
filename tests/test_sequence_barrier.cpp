#include <cassert>
#include <cstdint>
#include <iostream>

#include "ull/core/sequence_barrier.h"
#include "ull/core/single_producer_sequencer.h"

int main() {
  {
    ull::SingleProducerSequencer sequencer(4);
    ull::SequenceBarrier barrier(&sequencer);

    const auto seq = sequencer.next();

    // Before publish: consumer cannot see it.
    assert(!barrier.is_available(seq));

    // Publish: visibility boundary.
    sequencer.publish(seq);

    const auto available = barrier.wait_until_available(seq);
    assert(available >= seq);
    assert(barrier.is_available(seq));

    assert(!barrier.is_available(1));
  }
  {
    ull::SingleProducerSequencer sequencer(4);
    ull::BusySpinWaitStrategy strategy;
    ull::SequenceBarrier barrier(&sequencer, strategy);

    const auto seq = sequencer.next();

    assert(!barrier.is_available(seq));

    sequencer.publish(seq);

    barrier.wait_until_available(seq);
    assert(barrier.is_available(seq));
  }
  {
    ull::SingleProducerSequencer sequencer(4);
    ull::SpinWaitStrategy strategy(ull::util::SpinStrategy::ThreadYield);
    ull::SequenceBarrier<ull::SpinWaitStrategy> barrier(&sequencer, strategy);

    const auto seq = sequencer.next();

    assert(!barrier.is_available(seq));

    sequencer.publish(seq);

    barrier.wait_until_available(seq);
    assert(barrier.is_available(seq));
  }
  {
    ull::SingleProducerSequencer sequencer(8);
    ull::SequenceBarrier barrier(&sequencer);

    const auto s0 = sequencer.next();
    const auto s1 = sequencer.next();
    const auto s2 = sequencer.next();

    sequencer.publish(s2);

    const auto available = barrier.wait_until_available(s0);
    assert(available == s2);
    assert(barrier.is_available(s0));
    assert(barrier.is_available(s1));
    assert(barrier.is_available(s2));
  }

  std::cout << "test_sequence_barrier PASS\n";
  return 0;
}
