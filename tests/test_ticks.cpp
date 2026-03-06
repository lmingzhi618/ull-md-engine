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
  const auto dt = t1 - t0;
  const auto ns = ull::perf::ticks_to_ns(dt);
  assert(dt > 0);
  assert(ns > 0);

  // A longer sleep should generally produce a larger delta.
  const auto a0 = ull::perf::ticks();
  std::this_thread::sleep_for(std::chrono::milliseconds(1));
  const auto a1 = ull::perf::ticks();
  const auto ns1 = ull::perf::ticks_to_ns(a1 - a0);

  const auto b0 = ull::perf::ticks();
  std::this_thread::sleep_for(std::chrono::milliseconds(3));
  const auto b1 = ull::perf::ticks();
  const auto ns2 = ull::perf::ticks_to_ns(b1 - b0);

  assert(ns2 > ns1);
  std::cout << "test_ticks PASS\n";
  return 0;
}
