#include <cassert>
#include <cstdint>
#include <iostream>

#include "ull/core/sequenced_ring.h"

int main() {
  {
    ull::SequencedRing<std::uint64_t> ring(4);

    assert(ring.capacity() == 4);

    ring.write(0, 100);
    ring.write(1, 200);
    ring.write(2, 300);
    ring.write(3, 400);

    assert(ring.index(0) == 0);
    assert(ring.index(1) == 1);
    assert(ring.index(2) == 2);
    assert(ring.index(3) == 3);

    assert(ring.read(0) == 100);
    assert(ring.read(1) == 200);
    assert(ring.read(2) == 300);
    assert(ring.read(3) == 400);
  }
  {

    ull::SequencedRing<std::uint64_t> ring(4);

    ring.write(0, 100);
    assert(ring.read(0) == 100);

    // Slot reuse: seq 4 maps back to slot 0.
    // Safety is not decided by SequencedRing; it must be guaranteed by
    // sequencer/barrier/gating logic before this write happens.
    assert(ring.index(4) == 0);

    ring.write(4, 500);
    assert(ring.read(4) == 500);
    assert(ring.read(0) == 500);
  }

  std::cout << "test_sequenced_ring PASS\n";
  return 0;
}
