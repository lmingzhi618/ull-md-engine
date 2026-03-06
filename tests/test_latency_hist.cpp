#include <cassert>
#include <cstdint>
#include <iostream>
#include <string>

#include "ull/perf/latency_hist.h"

static bool contains(const std::string &s, const std::string &key) {
  return s.find(key) != std::string::npos;
}

int main() {
  // max=1000ns, bucket=10ns => 101 buckets
  ull::perf::LatencyHist hist(1000, 10);

  // cap behavior: values above max should not crash and should be counted
  hist.add(5000); // should cap to 1000
  hist.add(1000);
  hist.add(0);
  hist.add(10);

  const auto rep = hist.report();
  assert(contains(rep, "count="));

  assert(contains(rep, "p50_ns="));
  assert(contains(rep, "p99_ns="));
  assert(contains(rep, "p999_ns="));

  // Basic sanity: count should be 4
  assert(contains(rep, "count=4"));

  std::cout << "test_latency_hist PASS\n";
  return 0;
}
