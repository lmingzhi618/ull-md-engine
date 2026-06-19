#include <cassert>
#include <cstdint>
#include <iostream>

#include "ull/core/single_producer_sequencer.h"

int main() {
  {
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

    assert(!s.is_available(0));
    assert(!s.is_available(3));

    s.publish(3);
    assert(s.cursor() == 3);

    s.wait_until_available(3);

    assert(s.is_available(0));
    assert(s.is_available(3));
    assert(!s.is_available(4));
    // Consumer has processed seq 0, so slot 0 is reusable.
    s.mark_consumed(0);
    assert(s.remaining_capacity() == 1);

    assert(s.try_next(seq));
    assert(seq == 4);
    assert(s.remaining_capacity() == 0);
  }

  {
    ull::SingleProducerSequencer s(2);
    assert(s.next() == 0);
    assert(s.next() == 1);
    assert(s.remaining_capacity() == 0);

    s.mark_consumed(0);
    assert(s.next() == 2);
    assert(s.remaining_capacity() == 0);
  }

  {
    ull::SingleProducerSequencer s(4);

    const auto seq = s.next();
    assert(seq == 0);

    assert(!s.is_available(0));
    s.publish(0);

    s.wait_until_available(0);
    assert(s.is_available(0));
  }

  {
    ull::SingleProducerSequencer s(4);

    const auto s0 = s.next();
    const auto s1 = s.next();

    assert(s0 == 0);
    assert(s1 == 1);

    s.publish(s0);
    assert(s.cursor() == 0);
    assert(s.is_available(0));
    assert(!s.is_available(1));

    s.publish(s1);
    assert(s.cursor() == 1);
    assert(s.is_available(1));
  }

  std::cout << "test_single_producer_sequencer PASS\n";

  return 0;
}
