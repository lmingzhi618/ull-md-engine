#pragma once

namespace ull {
class BusySpinWaitStrategy {
public:
  void idle() const noexcept {
    // Busy-spin strategy v1.
    // Intentionally does nothing to avoid scheduler wakeup latency.
  }
};
} // namespace ull
