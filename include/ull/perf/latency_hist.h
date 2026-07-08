#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace ull::perf {

class LatencyHist {
public:
  LatencyHist(std::uint64_t max_ns, std::uint64_t bucket_ns);
  void add(std::uint64_t ns) noexcept;
  std::string report() const;

  std::uint64_t count() const noexcept;
  std::uint64_t p50_ns() const;
  std::uint64_t p99_ns() const;
  std::uint64_t p999_ns() const;

private:
  std::uint64_t max_;
  std::uint64_t bucket_;
  std::vector<std::uint64_t> buckets_;
  std::uint64_t count_{0};
};

} // namespace ull::perf
