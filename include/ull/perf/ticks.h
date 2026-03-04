#pragma once
#include <cstdint>

namespace ull::perf {

// Must be called once at program start.
void init_ticks();

// Return a monotonic tick.
std::uint64_t ticks() noexcept;

// Convert tick delta to nanoseconds.
std::uint64_t ticks_to_ns(std::uint64_t delta) noexcept;
} // namespace ull::perf
