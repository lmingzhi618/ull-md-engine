#pragma once

#include "ull/util/spin_wait.h"

namespace ull {
class BusySpinWaitStrategy {
public:
  void idle() const noexcept {
    // Busy-spin strategy v1.
    // Intentionally does nothing to avoid scheduler wakeup latency.
  }
  void reset() const noexcept {}
};

class SpinWaitStrategy {
public:
  explicit SpinWaitStrategy(
      util::SpinStrategy strategy = util::SpinStrategy::PureSpin) noexcept
      : spin_wait_(strategy) {}

  void idle() noexcept { spin_wait_.pause(); }

  void reset() noexcept { spin_wait_.reset(); }

private:
  util::SpinWait spin_wait_;
};
} // namespace ull
