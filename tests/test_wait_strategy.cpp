#include <iostream>

#include "ull/core/wait_strategy.h"

int main() {
  {
    ull::BusySpinWaitStrategy strategy;
    // Busy spin v1: idle intentionally does nothing.
    strategy.idle();
  }
  {
    // Default SpinWaitStrategy uses util::SpinWaitStrategy::PureSpin.
    ull::SpinWaitStrategy strategy;

    // Wait: one unsuccessful wait iteration.
    strategy.idle();

    // Reset: waiting succeed, reset adaptive/backoff state.
    strategy.reset();
  }
  {
    // Yield strategy: gives the scheduler a chance to run another thread.
    ull::SpinWaitStrategy strategy(ull::util::SpinStrategy::ThreadYield);
    strategy.idle();
    strategy.reset();
  }
  {
    // Adaptive strategy: adjusts spin/yield behavior based on wait length.
    ull::SpinWaitStrategy strategy(ull::util::SpinStrategy::AdaptiveSpinYield);

    strategy.idle();
    strategy.reset();
  }

  std::cout << "test_wait_strategy PASS\n";
  return 0;
}
