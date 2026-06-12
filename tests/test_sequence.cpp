#include <cassert>
#include <cstdint>
#include <iostream>

#include "ull/core/cacheline.h"
#include "ull/core/sequence.h"

int main() {
  ull::Sequence seq(10);

  assert(alignof(ull::Sequence) == ull::kCacheLine);
  assert(seq.load(std::memory_order_relaxed) == 10);

  seq.store(42, std::memory_order_relaxed);
  assert(seq.load(std::memory_order_relaxed) == 42);

  const auto old = seq.fetch_add(3, std::memory_order_relaxed);
  assert(old == 42);
  assert(seq.load(std::memory_order_relaxed) == 45);

  std::cout << "test_sequence PASS\n";

  return 0;
}
