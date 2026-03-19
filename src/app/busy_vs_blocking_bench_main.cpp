#include <atomic>
#include <cstdint>
#include <iostream>
#include <string>
#include <thread>

#include "ull/core/blocking_queue.h"
#include "ull/core/spsc_ring.h"
#include "ull/perf/latency_hist.h"
#include "ull/perf/ticks.h"
#include "ull/proto/simple_binary.h"

namespace {

constexpr std::uint64_t kMaxNs = 20'000'000;
constexpr std::uint64_t kBucketNs = 50;
constexpr std::size_t kQueueCapacity = 1u << 16;

inline void record_latency(ull::perf::LatencyHist &hist, ull::proto::Msg &m,
                           std::uint64_t &seen, std::uint32_t WARMUP) {
  const auto t1 = ull::perf::ticks();
  const auto dt_ns = ull::perf::ticks_to_ns(t1 - m.tsc_send);

  if (++seen > WARMUP) {
    hist.add(dt_ns);
  }
}

int run_busy_bench(std::uint32_t N, std::uint32_t WARMUP) {
  ull::perf::init_ticks();
  ull::SpscRing<ull::proto::Msg> q(kQueueCapacity);
  ull::perf::LatencyHist hist(kMaxNs, kBucketNs);

  std::atomic<bool> done{false};

  std::thread consumer([&] {
    ull::proto::Msg m{};
    std::uint64_t seen = 0;

    while (!done.load(std::memory_order_acquire)) {
      if (q.try_pop(m)) {
        record_latency(hist, m, seen, WARMUP);
      }
    }

    while (q.try_pop(m)) {
      record_latency(hist, m, seen, WARMUP);
    }
  });

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

  std::cout << "mode=busy" << " N=" << N << " warmup=" << WARMUP << std::endl;
  std::cout << "hist_unit=ns bucket_ns=" << kBucketNs << " max_ns=" << kMaxNs
            << std::endl;
  std::cout << hist.report();

  return 0;
}

int run_blocking_bench(std::uint32_t N, std::uint32_t WARMUP) {
  ull::perf::init_ticks();

  ull::BlockingQueue<ull::proto::Msg> q(kQueueCapacity);
  ull::perf::LatencyHist hist(kMaxNs, kBucketNs);

  std::thread consumer([&] {
    ull::proto::Msg m{};
    std::uint64_t seen = 0;

    while (q.pop(m)) {
      record_latency(hist, m, seen, WARMUP);
    }
  });

  for (std::uint32_t i = 1; i <= N; i++) {
    ull::proto::Msg m{};
    m.seq = i;
    m.msg_type = 1;
    m.payload = static_cast<std::uint64_t>(i) ^ 0xA5A5A5A5ULL;
    m.reserved = 0;
    m.tsc_send = ull::perf::ticks();

    q.push(m);
  }

  q.close();
  consumer.join();

  std::cout << "mode=blocking" << " N=" << N << " warmup=" << WARMUP
            << std::endl;
  std::cout << "hist_unit=ns bucket_ns=" << kBucketNs << " max_ns=" << kMaxNs
            << std::endl;
  std::cout << hist.report();

  return 0;
}
} // namespace

int main(int argc, char **argv) {
  const std::string mode = (argc >= 2) ? argv[1] : "busy";
  const std::uint32_t N =
      (argc >= 3) ? static_cast<std::uint32_t>(std::stoul(argv[2])) : 2'000'000;
  const std::uint32_t WARMUP =
      (argc >= 4) ? static_cast<std::uint32_t>(std::stoul(argv[3])) : 200'000;

  if (N <= WARMUP) {
    std::cerr << "N must be greater than WARMUP" << std::endl;
    return 1;
  }

  if (mode == "busy") {
    return run_busy_bench(N, WARMUP);
  }
  if (mode == "blocking") {

    return run_blocking_bench(N, WARMUP);
  }

  std::cerr << "unknown mode: " << mode << " (expected 'busy' or 'blocking')"
            << std::endl;
  return 1;
}
