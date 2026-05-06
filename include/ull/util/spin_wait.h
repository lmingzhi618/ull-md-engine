#pragma once

#include <cstdint>
#include <thread>

namespace ull::util {

enum class SpinStrategy {
  PureSpin,
  CpuRelax,
  ThreadYield,
  ExponentialBackoff,
  HybridSpinYield,
  AdaptiveSpinYield,
};

inline void cpu_relax_once() noexcept {
#if defined(__x86_64__) || defined(_M_X64)
  __builtin_ia32_pause();
#elif defined(__aarch64__) || defined(__arm64__)
  asm volatile("yield");
#else
  // Fallback: do nothing.
#endif
}

class SpinWait {
public:
  explicit SpinWait(SpinStrategy strategy) noexcept : strategy_(strategy) {}

  void pause() noexcept {
    switch (strategy_) {
    case SpinStrategy::PureSpin:
      break;
    case SpinStrategy::CpuRelax:
      cpu_relax_once();
      break;
    case SpinStrategy::ThreadYield:
      std::this_thread::yield();
      break;
    case SpinStrategy::ExponentialBackoff:
      backoff_pause();
      break;
    case SpinStrategy::HybridSpinYield:
      hybrid_pause();
      break;
    case SpinStrategy::AdaptiveSpinYield:
      adaptive_pause();
      break;
    }
    ++attempts_;
  }

  void reset() noexcept {
    // If we succeeded quickly, allow a bit more spinning next time.
    // If we had to wait long enough to yield, reduce spin budget.
    if (strategy_ == SpinStrategy::AdaptiveSpinYield) {
      if (attempts_ < adaptive_spin_limit_) {
        if (adaptive_spin_limit_ < kMaxAdaptiveSpinLimit) {
          ++adaptive_spin_limit_;
        }
      } else {
        if (adaptive_spin_limit_ > kMinAdaptiveSpinLimit) {
          --adaptive_spin_limit_;
        }
      }
    }

    attempts_ = 0;
  }

private:
  void backoff_pause() noexcept {
    if (attempts_ < 16) {
      cpu_relax_once();
      return;
    }

    if (attempts_ < 64) {
      for (std::uint32_t i = 0; i < 8; ++i) {
        cpu_relax_once();
      }
      return;
    }

    std::this_thread::yield();
  }

  void hybrid_pause() noexcept {
    constexpr std::int32_t kSpinLimit = 32;

    if (attempts_ < kSpinLimit) {
      cpu_relax_once();
    } else {
      std::this_thread::yield();
    }
  }

  void adaptive_pause() noexcept {
    if (attempts_ < adaptive_spin_limit_) {
      cpu_relax_once();
    } else {
      std::this_thread::yield();
    }
  }

private:
  static constexpr std::uint32_t kMinAdaptiveSpinLimit = 4;
  static constexpr std::uint32_t kMaxAdaptiveSpinLimit = 128;
  SpinStrategy strategy_;
  std::uint32_t attempts_{0};
  std::uint32_t adaptive_spin_limit_{32};
};

inline const char *to_string(SpinStrategy strategy) noexcept {
  switch (strategy) {
  case SpinStrategy::PureSpin:
    return "pure_spin";
  case SpinStrategy::CpuRelax:
    return "cpu_relax";
  case SpinStrategy::ThreadYield:
    return "thread_yield";
  case SpinStrategy::ExponentialBackoff:
    return "backoff";
  case SpinStrategy::HybridSpinYield:
    return "hybrid";
  case SpinStrategy::AdaptiveSpinYield:
    return "adaptive";
  }
  return "unknown";
}

inline SpinStrategy parse_spin_strategy(const std::string &s) {
  if (s == "pure_spin") {
    return SpinStrategy::PureSpin;
  }
  if (s == "cpu_relax") {
    return SpinStrategy::CpuRelax;
  }
  if (s == "thread_yield") {
    return SpinStrategy::ThreadYield;
  }
  if (s == "backoff") {
    return SpinStrategy::ExponentialBackoff;
  }
  if (s == "hybrid") {
    return SpinStrategy::HybridSpinYield;
  }
  if (s == "adaptive") {
    return SpinStrategy::AdaptiveSpinYield;
  }
  throw std::invalid_argument("unknown spin strategy (expected pure_spin, "
                              "cpu_relax, thread_yield, backoff, or adaptive");
}
} // namespace ull::util
