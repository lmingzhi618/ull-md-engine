#pragma once
#include <cstdint>

namespace ull::perf {

// Initializes the tick source.
// Must be called once before using ticks_to_ns() for reporting.
void init_ticks();

// Returns a monotonic tick value.
// The unit is intentionally unspecified.
// Only tick deltas should be interpreted, via ticks_to_ns().
std::uint64_t ticks() noexcept;

// Converts a tick delta to nanoseconds.
// Input must be a delta (for example, ticks() - earlier_ticks).
std::uint64_t ticks_to_ns(std::uint64_t delta) noexcept;
} // namespace ull::perf
