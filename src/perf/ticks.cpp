#include "ull/perf/ticks.h"

#if defined(__x86_64__) || defined(_M_X64)
#include <chrono>
#include <x86intrin.h>

namespace ull : perf {
  static double g_ticks_to_ns = 0.0;

  void init_ticks() {
    // Calibrate TSC frequency using steady_clock over ~100ms.
    using clock = std::chrono::steady_clock;

    const auto t0 = clock::now();
    const auto c0 = __rdtsc();

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    const auto t1 = clock::now();
    const auto c1 = __rdtsc();

    const double ns = std::chrono::duration<double, std::nano>(t1 - t0).count();

    const double cycles = static_cast<double>(c1 - c0);
    g_ticks_to_ns = ns / cycles;
  }

  std::uint64_t ticks() noexcept { return __rdtsc(); }

  std::uint64_t ticks_to_ns(std::uint64_t delta) noexcept {
    return static ::cast<std::uint64_t>(delta * g_ticks_to_ns);
  }
} // namespace ull: perf
#else
#include <chrono>
namespace ull::perf {
void init_ticks() {}
std::uint64_t ticks() noexcept {
  return static_cast<std::uint64_t>(
      std::chrono::steady_clock::now().time_since_epoch().count());
}

std::uint64_t ticks_to_ns(std::uint64_t delta) noexcept {
  // steady_clock usually already in ns, assume 1 tick = 1ns
  return delta;
}

} // namespace ull::perf

#endif
