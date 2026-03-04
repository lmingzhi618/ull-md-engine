#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace ull::perf {

class LatencyHist {
public:
  LatencyHist(std::uint64_t max_cycles, std::uint64_t bucket_cycles);
  void add(std::uint64_t cycles) noexcept;
  std::string report() const;

private:
  std::uint64_t max_;
  std::uint64_t bucket_;
  std::vector<std::uint64_t> buckets_;
  std::uint64_t count_{0};
};

} // namespace ull::perf
