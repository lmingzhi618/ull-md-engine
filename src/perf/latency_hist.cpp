#include "ull/perf/latency_hist.h"
#include <sstream>

namespace ull::perf {

LatencyHist::LatencyHist(std::uint64_t max_cycles, std::uint64_t bucket_cycles)
    : max_(max_cycles), bucket_(bucket_cycles),
      buckets_(static_cast<std::size_t>(max_cycles / bucket_cycles) + 1, 0) {}

void LatencyHist::add(std::uint64_t cycles) noexcept {
  const auto capped = (cycles > max_) ? max_ : cycles;
  const auto idx = static_cast<std::size_t>(capped / bucket_);
  ++buckets_[idx];
  ++count_;
}

static std::uint64_t percentile_cycles(const std::vector<std::uint64_t> &b,
                                       std::uint64_t total, double p,
                                       std::uint64_t bucket) {
  const std::uint64_t target = static_cast<std::uint64_t>(total * p);
  std::uint64_t run = 0;
  for (std::size_t i = 0; i < b.size(); i++) {
    run += b[i];
    if (run >= target)
      return static_cast<std::uint64_t>(i) * bucket;
  }
  return static_cast<std::uint64_t>(b.size() - 1) * bucket;
}

std::string LatencyHist::report() const {
  std::ostringstream oss;
  oss << "count=" << count_ << "\n";
  if (count_ == 0)
    return oss.str();

  oss << "p50_cycles=" << percentile_cycles(buckets_, count_, 0.50, bucket_)
      << "\n";
  oss << "p99_cycles=" << percentile_cycles(buckets_, count_, 0.99, bucket_)
      << "\n";
  oss << "p999_cycles=" << percentile_cycles(buckets_, count_, 0.999, bucket_)
      << "\n";
  return oss.str();
}
} // namespace ull::perf
