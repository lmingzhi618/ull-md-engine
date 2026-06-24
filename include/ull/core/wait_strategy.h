#pragma once

namespace ull {
class BusySpinWaitStrategy {
public:
  void idle() const noexcept {
    // Busy-spin strategy v1.
    // Intentionslly does nothing to avoid scheduler wakeup latency.
  }
};
} // namespace ull
