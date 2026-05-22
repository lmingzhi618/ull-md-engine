#include <atomic>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include "ull/core/mpsc_ring.h"
#include "ull/perf/latency_hist.h"
#include "ull/perf/ticks.h"

namespace {
constexpr std::uint64_t kMaxNs = 20'000'000;
constexpr std::uint64_t kBucketNs = 50;
constexpr std::size_t kQueueCapacity = 1u << 16;

struct Msg {
  std::uint64_t tsc_send;
  std::uint32_t producer_id;
  std::uint32_t seq;
};

void record_latency(ull::perf::LatencyHist &hist, const Msg &m,
                    std::uint64_t &seen, std::uint64_t warmup) {
  const auto t1 = ull::perf::ticks();
  const auto dt_ns = ull::perf::ticks_to_ns(t1 - m.tsc_send);

  if (++seen > warmup) {
    hist.add(dt_ns);
  }
}

int run_bench(std::uint32_t producers, std::uint32_t messages_per_producer,
              std::uint32_t warmup) {
  if (producers == 0) {
    std::cerr << "producers must be > 0\n";
    return 1;
  }

  const std::uint64_t total =
      static_cast<std::uint64_t>(producers) * messages_per_producer;
  if (total <= warmup) {
    std::cerr << "total messages must be greater than warmup\n";
    return 1;
  }

  ull::perf::init_ticks();

  ull::MpscRing<Msg> q(kQueueCapacity);
  ull::perf::LatencyHist hist(kMaxNs, kBucketNs);

  std::atomic<bool> start{false};
  std::atomic<std::uint64_t> consumed{0};

  std::vector<std::uint64_t> per_produer_counts(producers, 0);

  std::thread consumer([&] {
    Msg m{};
    std::uint64_t seen = 0;

    while (consumed.load(std::memory_order_relaxed) < total) {
      if (q.try_pop(m)) {
        record_latency(hist, m, seen, warmup);

        if (m.producer_id < producers) {
          ++per_produer_counts[m.producer_id];
        }

        consumed.fetch_add(1, std::memory_order_relaxed);
      }
    }
  });

  std::vector<std::thread> producer_threads;
  producer_threads.reserve(producers);

  for (std::uint32_t p = 0; p < producers; ++p) {
    producer_threads.emplace_back([&, p] {
      while (!start.load(std::memory_order_acquire)) {
        // spin until all producers are ready
      }

      for (std::uint32_t i = 0; i < messages_per_producer; ++i) {
        Msg m{};
        m.producer_id = p;
        m.seq = i;

        m.tsc_send = ull::perf::ticks();
        q.try_push(m);
      }
    });
  }

  const auto t0 = std::chrono::steady_clock::now();
  start.store(true, std::memory_order_release);

  for (auto &t : producer_threads) {
    t.join();
  }

  consumer.join();

  const auto t1 = std::chrono::steady_clock::now();
  const auto elapsed_ns =
      std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count();

  const double elapsed_s = static_cast<double>(elapsed_ns) / 1e9;
  const double throughput =
      elapsed_s > 0.0 ? static_cast<double>(total) / elapsed_s : 0.0;

  bool ok = true;
  for (std::uint32_t p = 0; p < producers; ++p) {
    if (per_produer_counts[p] != messages_per_producer) {
      ok = false;
    }
  }

  std::cout << "queue=mpsc_spin_push"
            << " producers=" << producers
            << " messages_per_producer=" << messages_per_producer
            << " total_messages=" << total << " warmup=" << warmup << "\n";

  std::cout << "elapsed_ns=" << elapsed_ns << "\n";
  std::cout << "throughput_msg_per_sec=" << throughput << "\n";
  std::cout << "counts_ok=" << (ok ? "true" : "false") << "\n";
  std::cout << "hist_unit=ns bucket_ns=" << kBucketNs << " max_ns=" << kMaxNs
            << "\n";
  std::cout << hist.report();

  return ok ? 0 : 2;
}

} // namespace

int main(int argc, char **argv) {
  const std::uint32_t producers =
      (argc >= 2) ? static_cast<std::uint32_t>(std::stoul(argv[1])) : 1;
  const std::uint32_t messages_per_producer =
      (argc >= 3) ? static_cast<std::uint32_t>(std::stoul(argv[2])) : 500'000;
  const std::uint32_t warmup =
      (argc >= 4) ? static_cast<std::uint32_t>(std::stoul(argv[3])) : 50'000;

  try {
    return run_bench(producers, messages_per_producer, warmup);
  } catch (const std::exception &e) {
    std::cerr << e.what() << "\n";
    return 1;
  }
}
