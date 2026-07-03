#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <thread>

#include "ull/core/sequence_barrier.h"
#include "ull/core/sequenced_ring.h"
#include "ull/core/single_producer_sequencer.h"
#include "ull/core/spsc_ring.h"
#include "ull/perf/latency_hist.h"

namespace {
constexpr std::uint64_t kDefaultConsumers = 2;
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
  const auto consumers =
      argc > 1 ? parse_arg(argv, 1, kDefaultConsumers) : kDefaultConsumers;
  const auto messages =
      argc > 2 ? parse_arg(argv, 2, kDefaultMessages) : kDefaultMessages;
  const auto capacity =
      argc > 3 ? parse_arg(argv, 3, kDefaultCapacity) : kDefaultCapacity;

  ull::SingleProducerSequencer sequencer(capacity);
  ull::SequencedRing<std::uint64_t> ring(capacity);
  ull::GatingSequences gating(consumers);

  sequencer.set_gating_sequences(&gating);

  std::vector<ull::perf::LatencyHist> hists;
  hists.reserve(consumers);
  for (std::uint64_t i = 0; i < consumers; ++i) {
    hists.emplace_back(kMaxNs, kBucketNs);
  }

  std::atomic<bool> start{false};

  std::vector<std::thread> consumer_threads;
  consumer_threads.reserve(consumers);

  const auto begin = Clock::now();

  for (std::uint64_t consumer_id = 0; consumer_id < consumers; ++consumer_id) {
    consumer_threads.emplace_back([&, consumer_id] {
      ull::SequenceBarrier barrier(&sequencer);

      while (!start.load(std::memory_order_acquire)) {
      }

      for (std::uint64_t expected = 0; expected < messages; ++expected) {
        // Fanout visibility boundary:
        // each consumer waits for the same published sequence stream.
        const auto available = barrier.wait_until_available(expected);
        if (available < expected) {
          std::cerr << "visibility error\n";
        }

        const auto sent_ns = ring.read(expected);
        if (expected >= kWarmup) {
          const auto latency_ns = now_ns() - sent_ns;
          hists[consumer_id].add(latency_ns);
        }

        // Fanout backpressure:
        // producer capacity is controlled by the slowest consumer sequence.
        gating.mark_consumed(consumer_id, expected);
      }
    });
  }

  std::thread producer([&] {
    start.store(true, std::memory_order_release);

    for (std::uint64_t i = 0; i < messages; ++i) {
      const auto seq = sequencer.next();
      ring.write(seq, now_ns());
      sequencer.publish(seq);
    }
  });

  producer.join();

  for (auto &t : consumer_threads) {
    t.join();
  }

  const auto end = Clock::now();
  const auto elapsed_ns = static_cast<std::uint64_t>(
      std::chrono::duration_cast<std::chrono::nanoseconds>(end - begin)
          .count());

  const auto measured_messages =
      messages > kWarmup ? messages - kWarmup : std::uint64_t{0};
  const auto elapsed_s = static_cast<double>(elapsed_ns) / 1e9;
  const double throughput =
      elapsed_s > 0.0 ? static_cast<double>(messages) / elapsed_s : 0.0;

  std::cout << "queue=single_producer_fanout\n";
  std::cout << "consumers=" << consumers << "\n";
  std::cout << "messages=" << messages << "\n";
  std::cout << "capacity=" << capacity << "\n";
  std::cout << "warmup=" << kWarmup << "\n";
  std::cout << "measured_messages=" << measured_messages << "\n";
  std::cout << "elapsed_ns=" << elapsed_ns << "\n";
  std::cout << "throughput_msg_per_sec=" << throughput << "\n";
  std::cout << "hist_unit=ns bucket_ns=" << kBucketNs << " max_ns=" << kMaxNs
            << "\n";

  for (std::uint64_t consumer_id = 0; consumer_id < consumers; ++consumer_id) {
    std::cout << "consumer=" << consumer_id << "\n";
    std::cout << hists[consumer_id].report();
  }
  return 0;
}
