#include "ull/core/wait_strategy.h"
#include <iostream>

int main() {
  ull::BusySpinWaitStrategy strategy;
  strategy.idle();

  std::cout << "test_wait_strategy PASS\n";

  return 0;
}
