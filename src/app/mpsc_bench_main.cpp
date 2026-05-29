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

enum class BenchMode {
  Push,
  TryDrop,
};

BenchMode parse_mode(const std::string &s) {
  if (s == "push") {
    return BenchMode::Push;
  }
  if (s == "try_drop") {
    return BenchMode::TryDrop;
  }
  throw std::invalid_argument("mode must be 'push' or 'try_drop'");
}

const char *to_string(BenchMode mode) {
  switch (mode) {
  case BenchMode::Push:
    return "push";
  case BenchMode::TryDrop:
    return "try_drop";
  }
  return "unknown";
}

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

int run_bench(BenchMode mode, std::uint32_t producers,
              std::uint32_t messages_per_producer, std::uint32_t warmup,
              std::size_t queue_capacity) {
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

  ull::MpscRing<Msg> q(queue_capacity);
  ull::perf::LatencyHist hist(kMaxNs, kBucketNs);

  std::vector<ull::perf::LatencyHist> push_hists;
  push_hists.reserve(producers);

  for (std::uint32_t p = 0; p < producers; ++p) {
    push_hists.emplace_back(kMaxNs, kBucketNs);
  }

  std::atomic<bool> start{false};
  std::atomic<bool> producers_done{false};
  std::atomic<std::uint64_t> published{0};
  std::atomic<std::uint64_t> dropped{0};
  std::atomic<std::uint64_t> consumed{0};

  std::vector<std::uint64_t> per_producer_counts(producers, 0);

  std::thread consumer([&] {
    Msg m{};
    std::uint64_t seen = 0;

    while (!producers_done.load(std::memory_order_acquire) ||
           consumed.load(std::memory_order_relaxed) <
               published.load(std::memory_order_acquire)) {
      if (q.try_pop(m)) {
        record_latency(hist, m, seen, warmup);
        if (m.producer_id < producers) {
          ++per_producer_counts[m.producer_id];
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

      std::uint64_t produced_seen = 0;
      const std::uint64_t producer_warmup = warmup / producers;

      for (std::uint32_t i = 0; i < messages_per_producer; ++i) {
        Msg m{};
        m.producer_id = p;
        m.seq = i;

        const auto push_start = ull::perf::ticks();
        m.tsc_send = push_start;
        bool did_publish = false;

        if (mode == BenchMode::Push) {
          q.push(m);
          did_publish = true;
        } else {
          did_publish = q.try_push(m);
        }

        const auto push_end = ull::perf::ticks();
        const auto push_ns = ull::perf::ticks_to_ns(push_end - push_start);

        if (did_publish) {
          published.fetch_add(1, std::memory_order_release);

          if (++produced_seen > producer_warmup) {
            push_hists[p].add(push_ns);
          }
        } else {
          dropped.fetch_add(1, std::memory_order_relaxed);
        }
      }
    });
  }

  const auto t0 = std::chrono::steady_clock::now();
  start.store(true, std::memory_order_release);

  for (auto &t : producer_threads) {
    t.join();
  }
  producers_done.store(true, std::memory_order_release);
  consumer.join();

  const auto t1 = std::chrono::steady_clock::now();
  const auto elapsed_ns =
      std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count();

  const double elapsed_s = static_cast<double>(elapsed_ns) / 1e9;
  const double throughput =
      elapsed_s > 0.0 ? static_cast<double>(total) / elapsed_s : 0.0;

  bool ok = true;
  const auto expected_per_producer =
      (mode == BenchMode::Push) ? messages_per_producer : 0;

  if (mode == BenchMode::Push) {
    for (std::uint32_t p = 0; p < producers; ++p) {
      if (per_producer_counts[p] != expected_per_producer) {
        ok = false;
      }
    }
  } else {
    ok = (consumed.load(std::memory_order_relaxed) ==
          published.load(std::memory_order_relaxed));
  }

  std::cout << "queue=mpsc"
            << " mode=" << to_string(mode) << " producers=" << producers
            << " messages_per_producer=" << messages_per_producer
            << " total_messages=" << total << " warmup=" << warmup
            << " capacity=" << queue_capacity << "\n";

  std::cout << "published=" << published.load(std::memory_order_relaxed)
            << "\n";
  std::cout << "dropped=" << dropped.load(std::memory_order_relaxed) << "\n";
  std::cout << "consumed=" << consumed.load(std::memory_order_relaxed) << "\n";

  std::cout << "elapsed_ns=" << elapsed_ns << "\n";
  std::cout << "throughput_msg_per_sec=" << throughput << "\n";
  std::cout << "integrity_ok=" << (ok ? "true" : "false") << "\n";
  std::cout << "hist_unit=ns bucket_ns=" << kBucketNs << " max_ns=" << kMaxNs
            << "\n";

  std::cout << hist.report();
  for (std::uint32_t p = 0; p < producers; ++p) {
    std::cout << "push_latency_producer=" << p << "\n";
    std::cout << push_hists[p].report();
  }
  return ok ? 0 : 2;
}

} // namespace

int main(int argc, char **argv) {
  const BenchMode mode = (argc >= 2) ? parse_mode(argv[1]) : BenchMode::Push;
  const std::uint32_t producers =
      (argc >= 3) ? static_cast<std::uint32_t>(std::stoul(argv[2])) : 1;
  const std::uint32_t messages_per_producer =
      (argc >= 4) ? static_cast<std::uint32_t>(std::stoul(argv[3])) : 500'000;
  const std::uint32_t warmup =
      (argc >= 5) ? static_cast<std::uint32_t>(std::stoul(argv[4])) : 50'000;
  const std::uint32_t queue_capacity =
      (argc >= 6) ? static_cast<std::size_t>(std::stoul(argv[5])) : (1u << 16);
  try {
    return run_bench(mode, producers, messages_per_producer, warmup,
                     queue_capacity);
  } catch (const std::exception &e) {
    std::cerr << e.what() << "\n";
    return 1;
  }
}
