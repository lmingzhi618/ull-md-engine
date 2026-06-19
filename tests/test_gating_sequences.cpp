#include <cassert>
#include <cstdint>
#include <iostream>

#include "ull/core/gating_sequences.h"

int main() {
  {
    ull::GatingSequences g(3);

    assert(g.count() == 3);
    assert(g.load_min() == ull::kNoConsumerProgress);

    g.store(0, 10);
    g.store(1, 8);
    g.store(2, 12);

    assert(g.load_min() == 8);

    g.store(1, 11);
    assert(g.load_min() == 10);

    std::cout << "test_gating_sequences PASS\n";
  }

  {
    ull::GatingSequences g(3);

    g.store(0, 3);
    g.store(1, 1);
    g.store(2, 2);

    assert(g.load_min() == 1);

    // Fast consumers moving ahead do not increase safe capacity.
    g.store(0, 10);
    g.store(2, 9);
    assert(g.load_min() == 1);

    // Capacity only improves when the slowest consumer advances.
    g.store(1, 8);
    assert(g.load_min() == 8);
  }

  {
    ull::GatingSequences g(2);
    assert(g.load(99) == ull::kNoConsumerProgress);

    g.store(99, 123);
    assert(g.load_min() == ull::kNoConsumerProgress);
  }

  return 0;
}
