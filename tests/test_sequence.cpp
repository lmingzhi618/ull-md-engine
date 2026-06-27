#include <cassert>
#include <cstdint>
#include <iostream>

#include "ull/core/cacheline.h"
#include "ull/core/sequence.h"
#include "ull/core/sequence_barrier.h"

int main() {
  {
    ull::Sequence seq(10);

    assert(alignof(ull::Sequence) == ull::kCacheLine);
    assert(seq.load(std::memory_order_relaxed) == 10);

    seq.store(42, std::memory_order_relaxed);
    assert(seq.load(std::memory_order_relaxed) == 42);

    const auto old = seq.fetch_add(3, std::memory_order_relaxed);
    assert(old == 42);
    assert(seq.load(std::memory_order_relaxed) == 45);

    std::uint64_t expected = 45;
    assert(seq.compare_exchange_weak(expected, 100, std::memory_order_relaxed,
                                     std::memory_order_relaxed));

    assert(seq.load(std::memory_order_relaxed) == 100);
    assert(expected == 45);

    expected = 45;
    assert(!seq.compare_exchange_weak(expected, 200, std::memory_order_relaxed,
                                      std::memory_order_relaxed));

    assert(expected == 100);
    assert(seq.load(std::memory_order_relaxed) == 100);

    std::cout << "test_sequence PASS\n";
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

  return 0;
}
