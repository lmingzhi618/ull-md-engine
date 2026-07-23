#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace ull::bench {

enum class CjmEventLevel {
    Info,
    Warning,
    Error,
};

struct CjmMappingDetail {
    std::int64_t signed_delta; // json:"signed_delta"
    double ratio;              // json:"ratio"
};

struct CjmMappingSurfaceEvent {
    CjmEventLevel level;                   // json:"level"
    CjmMappingDetail detail;               // json:"detail"
    std::optional<std::string> empty_note; // json:"empty_note,omitempty"
    std::vector<std::int64_t> samples;     // json:"samples"
    std::unordered_map<std::string, std::uint64_t> counters; // json:"counters"
    std::string internal_debug;                              // json:"-"
};

} // namespace ull::bench
