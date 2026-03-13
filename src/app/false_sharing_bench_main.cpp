#include <atomic>
#include <cstdint>
#include <iostream>
#include <string>
#include <thread>

#include "ull/core/spsc_ring.h"
#include "ull/core/spsc_ring_unpadded.h"
#include "ull/perf/latency_hist.h"
#include "ull/perf/ticks.h"
#include "ull/proto/simple_binary.h"

template <typename Ring>
int run_bench(const std::string &mode, std::uint32_t N, std::uint32_t WARMUP) {
  ull::perf::init_ticks();
  Ring q(1u << 16);

  constexpr std::uint64_t kMaxNs = 20'000'000;
  constexpr std::uint64_t kBucketNs = 50;
  ull::perf::LatencyHist hist(kMaxNs, kBucketNs);

  std::atomic<bool> start{false};
  std::atomic<bool> done{false};

  std::thread consumer([&] {
    while (!start.load(std::memory_order_acquire)) {
    }
    ull::proto::Msg m{};
    std::uint64_t seen = 0;

    while (!done.load(std::memory_order_acquire)) {
      while (q.try_pop(m)) {
        const auto t1 = ull::perf::ticks();
        const auto dt_ns = ull::perf::ticks_to_ns(t1 - m.tsc_send);

        ++seen;
        if (seen > WARMUP) {
          hist.add(dt_ns);
        }
      }
    }

    while (q.try_pop(m)) {
      const auto t1 = ull::perf::ticks();
      const auto dt_ns = ull::perf::ticks_to_ns(t1 - m.tsc_send);

      ++seen;
      if (seen > WARMUP) {
        hist.add(dt_ns);
      }
    }
  });

  start.store(true, std::memory_order_release);
  for (std::uint32_t i = 1; i <= N; i++) {
    ull::proto::Msg m{};
    m.seq = i;
    m.msg_type = 1;
    m.payload = static_cast<std::uint64_t>(i) ^ 0xA5A5A5A5ULL;
    m.reserved = 0;

    while (true) {
      m.tsc_send = ull::perf::ticks();
      if (q.try_push(m)) {
        break;
      }
    }
  }
  done.store(true, std::memory_order_release);
  consumer.join();

  std::cout << "mode=" << mode << " N=" << N << " warmup=" << WARMUP
            << std::endl;
  std::cout << "hist_unit=ns bucket_ns=" << kBucketNs << " max_ns=" << kMaxNs
            << std::endl;
  std::cout << hist.report();

  return 0;
}

int main(int argc, char **argv) {
  const std::string mode = (argc >= 2) ? argv[1] : "padded";
  const std::uint32_t N =
      (argc >= 3) ? static_cast<std::uint32_t>(std::stoul(argv[2])) : 2'000'000;
  const std::uint32_t WARMUP =
      (argc >= 4) ? static_cast<std::uint32_t>(std::stoul(argv[3])) : 200'000;

  if (N <= WARMUP) {
    std::cerr << "N must be greater than WARMUP\n";
    return 1;
  }

  if (mode == "padded") {
    return run_bench<ull::SpscRing<ull::proto::Msg>>(mode, N, WARMUP);
  }

  if (mode == "unpadded") {
    return run_bench<ull::SpscRingUnpadded<ull::proto::Msg>>(mode, N, WARMUP);
  }

  std::cerr << "unknown mode: " << mode
            << " (expected 'padded' or 'unpadded')\n";
  return 1;
}
