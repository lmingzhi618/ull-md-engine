#include <cassert>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <thread>

#include "ull/perf/ticks.h"

int main() {
  ull::perf::init_ticks();

  // Ticks should be monotonic (non-descreasing) in the same thread
  std::uint64_t last = ull::perf::ticks();
  for (int i = 0; i < 10000; i++) {
    const auto now = ull::perf::ticks();
    assert(now >= last);
    last = now;
  }

  // ticks_to_ns(0) must be 0
  assert(ull::perf::ticks_to_ns(0) == 0);

  // Smoke: sleep should produce a positive delta in ns
  const auto t0 = ull::perf::ticks();
  std::this_thread::sleep_for(std::chrono::milliseconds(1));
  const auto t1 = ull::perf::ticks();
  const auto ns = ull::perf::ticks_to_ns(t1 - t0);
  assert(ns > 0);

  std::cout << "test_ticks PASS\n";
  return 0;
}
