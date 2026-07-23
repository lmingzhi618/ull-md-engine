#pragma once

#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace ull::bench {

struct BenchmarkLogEvent {
    std::string benchmark_name;                    // json:"benchmark_name"
    std::string phase;                             // json:"phase"
    std::uint64_t timestamp_ns;                    // json:"timestamp_ns"
    std::uint64_t elapsed_ns;                      // json:"elapsed_ns"
    bool success;                                  // json:"success"
    std::optional<std::string> note;               // json:"note,omitempty"
    std::vector<std::string> tags;                 // json:"tags"
    std::map<std::string, std::uint64_t> counters; // json:"counters"
};
} // namespace ull::bench
