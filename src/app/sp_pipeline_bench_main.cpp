#include <atomic>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <thread>

#include "ull/core/single_producer_event_pipeline.h"
#include "ull/perf/latency_hist.h"

namespace {
constexpr std::uint64_t kDefaultMessages = 1000000;
constexpr std::uint64_t kDefaultCapacity = 1024;
constexpr std::uint64_t kWarmup = 50000;

constexpr std::uint64_t kMaxNs = 20000000;
constexpr std::uint64_t kBucketNs = 50;

using Clock = std::chrono::steady_clock;

std::uint64_t now_ns() {
  return static_cast<std::uint64_t>(
      std::chrono::duration_cast<std::chrono::nanoseconds>(
          Clock::now().time_since_epoch())
          .count());
}

std::uint64_t parse_arg(char **argv, int index, std::uint64_t fallback) {
  if (argv[index] == nullptr) {
    return fallback;
  }
  return static_cast<std::uint64_t>(std::strtoull(argv[index], nullptr, 10));
}

} // namespace

int main(int argc, char **argv) {

  const auto messages =
      argc > 1 ? parse_arg(argv, 1, kDefaultMessages) : kDefaultMessages;
  const auto capacity =
      argc > 2 ? parse_arg(argv, 2, kDefaultCapacity) : kDefaultCapacity;

  ull::SingleProducerEventPipeline<std::uint64_t> pipeline(capacity);
  ull::perf::LatencyHist hist(kMaxNs, kBucketNs);

  std::atomic<bool> start{false};

  const auto begin = Clock::now();

  std::thread consumer([&] {
    while (!start.load(std::memory_order_acquire)) {
    }

    for (std::uint64_t expected = 0; expected < messages; ++expected) {
      const auto available = pipeline.wait_until_available(expected);
      if (available < expected) {
        std::cerr << "visibility error\n";
        std::abort();
      }

      const auto sent_ns = pipeline.read(expected);
      if (expected >= kWarmup) {
        const auto latency_ns = now_ns() - sent_ns;
        hist.add(latency_ns);
      }

      pipeline.mark_consumed(expected);
    }
  });

  std::thread producer([&] {
    start.store(true, std::memory_order_release);

    for (std::uint64_t i = 0; i < messages; ++i) {
      const auto seq = pipeline.next();
      pipeline.write(seq, now_ns());
      pipeline.publish(seq);
    }
  });

  producer.join();
  consumer.join();

  const auto end = Clock::now();
  const auto elapsed_ns = static_cast<std::uint64_t>(
      std::chrono::duration_cast<std::chrono::nanoseconds>(end - begin)
          .count());

  const auto measured_messages =
      messages > kWarmup ? messages - kWarmup : std::uint64_t{0};
  const double elapsed_s = static_cast<double>(elapsed_ns) / 1e9;
  const double throughput =
      elapsed_s > 0.0 ? static_cast<double>(messages) / elapsed_s : 0.0;

  std::cout << "queue=single_producer_event_pipeline\n";
  std::cout << "messages=" << messages << "\n";
  std::cout << "capacity=" << capacity << "\n";
  std::cout << "warmup=" << kWarmup << "\n";
  std::cout << "measured_messages=" << measured_messages << "\n";
  std::cout << "elapsed_ns=" << elapsed_ns << "\n";
  std::cout << "throughput_msg_per_sec=" << throughput << "\n";
  std::cout << "hist_unit=ns bucket_ns=" << kBucketNs << " max_ns=" << kMaxNs
            << "\n";
  std::cout << hist.report();
  return 0;
}
