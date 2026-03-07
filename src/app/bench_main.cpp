#include <atomic>
#include <iostream>
#include <thread>

#include "ull/core/spsc_ring.h"
#include "ull/perf/latency_hist.h"
#include "ull/perf/ticks.h"
#include "ull/proto/simple_binary.h"

int main(int argc, char **argv) {
  // 1. Parse args (N, warmup)
  const std::uint32_t N =
      (argc >= 2) ? static_cast<std::uint32_t>(std::stoul(argv[1])) : 2'000'000;
  const std::uint32_t WARMUP =
      (argc >= 3) ? static_cast<std::uint32_t>(std::stoul(argv[2])) : 200'000;

  // 2. Init tick source (x86 will calibrate TSC -> ns)
  ull::perf::init_ticks();

  // 3. Data path: SPSC queue + hisogram (uints: ns)
  ull::SpscRing<ull::proto::Msg> q(1u << 16);

  // Histogram config (nanoseconds)
  // max_ns: cap extreme outliers; bucket_ns: resolution
  constexpr std::uint64_t kMaxNs = 5'000'000; // 5 ms cap (tune later)
  constexpr std::uint64_t kBucketNs = 50;     // 50 ns bucket
  ull::perf::LatencyHist hist(kMaxNs, kBucketNs);

  std::atomic<bool> done{false};

  // 4. Consumer thread: pop and record latency
  std::thread consumer([&] {
    try {
      ull::proto::Msg m{};
      std::uint64_t seen = 0;

      // Consume until producer signals done
      while (!done.load(std::memory_order_acquire)) {
        while (q.try_pop(m)) {
          const auto t1 = ull::perf::ticks();
          const auto dt_ticks = t1 - m.tsc_send;

          // Warn-up; drop first WARMUP samples to avoid cold-start noise
          ++seen;
          if (seen > WARMUP) {
            const auto dt_ns = ull::perf::ticks_to_ns(dt_ticks);
            hist.add(dt_ns);
          }
        }
        // MVP: pure busy-poll. v0.2 will add backoff/affinity.
      }

      // Drain remaining items after done=true
      while (q.try_pop(m)) {
        const auto t1 = ull::perf::ticks();
        const auto dt_ticks = t1 - m.tsc_send;

        ++seen;
        if (seen > WARMUP) {
          const auto dt_ns = ull::perf::ticks_to_ns(dt_ticks);
          hist.add(dt_ns);
        }
      }
    } catch (const std::exception &e) {
      std::cerr << "consumer exception: " << e.what() << std::endl;
      std::abort();
    } catch (...) {
      std::cerr << "consumer unknown exception\n";
      std::abort();
    }
  });

  // 5. Producer loop (main thread)
  for (std::uint32_t i = 1; i <= N; i++) {
    ull::proto::Msg m{};
    m.tsc_send = ull::perf::ticks(); // tick at send
    m.seq = i;
    m.payload = i * 2654435761u;

    // Spin until push succeds (MVP).
    // v0.2: consider backpressure strategies, pause/yield, drop policy.
    while (!q.try_push(m)) {
      // spin
    }
  }

  // Signal consumer to exit and join
  done.store(true, std::memory_order_release);
  consumer.join();

  // 6. Report
  std::cout << "N=" << N << " warmup=" << WARMUP << "\n";
  std::cout << "hist_unit=ns bucket_ns=" << kBucketNs << " max_ns=" << kMaxNs
            << "\n";
  std::cout << hist.report();
  return 0;
}
