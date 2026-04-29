#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>

#include "ull/core/spsc_ring.h"
#include "ull/perf/latency_hist.h"
#include "ull/perf/ticks.h"
#include "ull/proto/simple_binary.h"
#include "ull/util/spin_wait.h"

namespace {
constexpr std::uint64_t kMaxNs = 20'000'000;
constexpr std::uint64_t kBucketNs = 50;
constexpr std::size_t kQueueCapacity = 1u << 16;

inline void record_latency(ull::perf::LatencyHist &hist,
                           const ull::proto::Msg &m, std::uint64_t &seen,
                           std::uint32_t warmup) {
  const auto t1 = ull::perf::ticks();
  const auto dt_ns = ull::perf::ticks_to_ns(t1 - m.tsc_send);

  if (++seen > warmup) {
    hist.add(dt_ns);
  }
}

int run_bench(ull::util::SpinStrategy strategy, std::uint32_t N,
              std::uint32_t WARMUP) {
  if (N <= WARMUP) {
    std::cerr << "N must be greater than WARMUP\n";
    return 1;
  }
  ull::perf::init_ticks();
  ull::SpscRing<ull::proto::Msg> q(kQueueCapacity);
  ull::perf::LatencyHist hist(kMaxNs, kBucketNs);

  std::atomic<bool> done{false};

  std::thread consumer([&] {
    ull::proto::Msg m{};
    std::uint64_t seen = 0;
    ull::util::SpinWait wait(strategy);

    while (!done.load(std::memory_order_acquire)) {
      if (q.try_pop(m)) {
        wait.reset();
        record_latency(hist, m, seen, WARMUP);
      } else {
        wait.pause();
      }
    }

    while (q.try_pop(m)) {
      record_latency(hist, m, seen, WARMUP);
    }
  });

  ull::util::SpinWait wait(strategy);

  for (std::uint32_t i = 1; i <= N; i++) {
    ull::proto::Msg m{};
    m.seq = i;
    m.msg_type = 1;
    m.payload = static_cast<std::uint64_t>(i) & 0xA5A5A5A5ULL;
    m.reserved = 0;
    while (true) {
      m.tsc_send = ull::perf::ticks();
      if (q.try_push(m)) {
        wait.reset();
        break;
      }
      wait.pause();
    }
  }

  done.store(true, std::memory_order_release);
  consumer.join();

  std::cout << "strategy=" << ull::util::to_string(strategy) << " N=" << N
            << " warmup=" << WARMUP << "\n";
  std::cout << "hist_unit=ns bucket_ns=" << kBucketNs << " max_ns=" << kMaxNs
            << "\n";
  std::cout << hist.report();
  return 0;
}
} // namespace

int main(int argc, char **argv) {
  const std::string strategy_str = (argc >= 2) ? argv[1] : "pure_spin";

  const std::uint32_t N =
      (argc >= 3) ? static_cast<std::uint32_t>(std::stoul(argv[2])) : 2'000'000;
  const std::uint32_t WARMUP =
      (argc >= 4) ? static_cast<std::uint32_t>(std::stoul(argv[3])) : 200'000;

  try {
    const auto strategy = ull::util::parse_spin_strategy(strategy_str);
    return run_bench(strategy, N, WARMUP);
  } catch (const std::exception &e) {
    std::cerr << e.what() << "\n";
    return 1;
  }
}
