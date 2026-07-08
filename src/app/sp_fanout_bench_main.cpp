#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include <nlohmann/json.hpp>

#include "ull/core/sequence_barrier.h"
#include "ull/core/sequenced_ring.h"
#include "ull/core/single_producer_sequencer.h"
#include "ull/perf/latency_hist.h"
#include "ull/perf/thread_affinity.h"

namespace {
constexpr std::uint64_t kDefaultConsumers = 2;
constexpr std::uint64_t kDefaultMessages = 1000000;
constexpr std::uint64_t kDefaultCapacity = 1024;
constexpr std::uint64_t kDefaultWarmup = 50000;

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

void print_usage(const char *prog) {
  std::cerr
      << "usage: " << prog
      << " <consumers> <messages> <capacity> <affinity> <warmup> [--json]\n"
      << " consumers: positive integer\n"
      << " messages : positive integer\n"
      << " capacity : power of two\n"
      << " affinity : default | same | split\n"
      << " warmup   : non-negative integer smaller than messages\n"
      << " --json   : emit canonical benchmark JSON v1\n";
}

bool is_power_of_two(std::uint64_t x) noexcept {
  return x != 0 && (x & (x - 1)) == 0;
}

bool is_valid_affinity(const std::string &affinity) {
  return affinity == "default" || affinity == "same" || affinity == "split";
}

struct ConsumerLatencyResult {
  std::uint64_t consumer_id;
  std::uint64_t count;
  std::uint64_t p50_ns;
  std::uint64_t p99_ns;
  std::uint64_t p999_ns;
};

struct FanoutBenchResult {
  std::uint64_t consumers;
  std::uint64_t messages;
  std::uint64_t capacity;
  std::uint64_t warmup;
  std::uint64_t measured_messages;
  std::uint64_t elapsed_ns;
  double throughput_msg_per_sec;
  std::string affinity;
  std::vector<ConsumerLatencyResult> latency;
};

void to_json(nlohmann::json &j, const ConsumerLatencyResult &result) {
  j = nlohmann::json{
      {"consumer", result.consumer_id}, {"count", result.count},
      {"p50_ns", result.p50_ns},        {"p99_ns", result.p99_ns},
      {"p999_ns", result.p999_ns},
  };
}

void to_json(nlohmann::json &j, const FanoutBenchResult &result) {
  j = nlohmann::json{
      {"schema_version", "1.0"},
      {"benchmark",
       {{"name", "sp_fanout_bench"},
        {"category", "concurrency"},
        {"version", "0.1"}}},
      {"configuration",
       {{"consumers", result.consumers},
        {"messages", result.messages},
        {"capacity", result.capacity},
        {"affinity", result.affinity},
        {"warmup", result.warmup}}},
      {"measurement",
       {{"measured_messages", result.measured_messages},
        {"elapsed_ns", result.elapsed_ns},
        {"throughput_msg_per_sec", result.throughput_msg_per_sec}}},
      {"latency",
       {{"unit", "ns"},
        {"bucket_ns", kBucketNs},
        {"max_ns", kMaxNs},
        {"consumers", result.latency}}},
      {"notes", {"macOS affinity is best-effort and not strict CPU pinning"}},
  };
}

void print_json(std::ostream &out, const FanoutBenchResult &result) {
  const nlohmann::json j = result;
  out << j.dump(2) << "\n";
}
} // namespace

int main(int argc, char **argv) {
  const auto consumers =
      argc > 1 ? parse_arg(argv, 1, kDefaultConsumers) : kDefaultConsumers;
  const auto messages =
      argc > 2 ? parse_arg(argv, 2, kDefaultMessages) : kDefaultMessages;
  const auto capacity =
      argc > 3 ? parse_arg(argv, 3, kDefaultCapacity) : kDefaultCapacity;
  const std::string affinity = argc > 4 ? argv[4] : "default";
  const auto warmup =
      argc > 5 ? parse_arg(argv, 5, kDefaultWarmup) : kDefaultWarmup;
  const bool json_output = argc > 6 && std::string(argv[6]) == "--json";

  if (argc > 7 || (argc > 6 && !json_output)) {
    print_usage(argv[0]);
    return 1;
  }

  if (consumers == 0 || messages == 0 || !is_power_of_two(capacity) ||
      !is_valid_affinity(affinity) || warmup >= messages) {
    print_usage(argv[0]);
    return 1;
  }

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
          std::abort();
        }

        const auto sent_ns = ring.read(expected);
        if (expected >= warmup) {
          const auto latency_ns = now_ns() - sent_ns;
          hists[consumer_id].add(latency_ns);
        }

        // Fanout backpressure:
        // producer capacity is controlled by the slowest consumer sequence.
        gating.mark_consumed(consumer_id, expected);
      }
    });

    if (affinity == "same") {
      ull::perf::pin_thread(consumer_threads.back(), 0);
    } else if (affinity == "split") {
      ull::perf::pin_thread(consumer_threads.back(),
                            static_cast<std::size_t>(consumer_id + 1));
    }
  }

  std::thread producer([&] {
    start.store(true, std::memory_order_release);

    for (std::uint64_t i = 0; i < messages; ++i) {
      const auto seq = sequencer.next();
      ring.write(seq, now_ns());
      sequencer.publish(seq);
    }
  });
  if (affinity == "same" || affinity == "split") {
    ull::perf::pin_thread(producer, 0);
  }

  producer.join();

  for (auto &t : consumer_threads) {
    t.join();
  }

  const auto end = Clock::now();
  const auto elapsed_ns = static_cast<std::uint64_t>(
      std::chrono::duration_cast<std::chrono::nanoseconds>(end - begin)
          .count());

  const auto measured_messages =
      messages > warmup ? messages - warmup : std::uint64_t{0};
  const auto elapsed_s = static_cast<double>(elapsed_ns) / 1e9;
  const double throughput =
      elapsed_s > 0.0 ? static_cast<double>(messages) / elapsed_s : 0.0;

  FanoutBenchResult result{
      .consumers = consumers,
      .messages = messages,
      .capacity = capacity,
      .warmup = warmup,
      .measured_messages = measured_messages,
      .elapsed_ns = elapsed_ns,
      .throughput_msg_per_sec = throughput,
      .affinity = affinity,
      .latency = {},
  };
  result.latency.reserve(consumers);
  for (std::uint64_t consumer_id = 0; consumer_id < consumers; ++consumer_id) {
    const auto &hist = hists[consumer_id];

    result.latency.push_back({
        .consumer_id = consumer_id,
        .count = hist.count(),
        .p50_ns = hist.p50_ns(),
        .p99_ns = hist.p99_ns(),
        .p999_ns = hist.p999_ns(),
    });
  }

  if (json_output) {
    print_json(std::cout, result);
  } else {
    std::cout << "queue=single_producer_fanout\n";
    std::cout << "affinity=" << affinity << "\n";
    std::cout << "consumers=" << consumers << "\n";
    std::cout << "messages=" << messages << "\n";
    std::cout << "capacity=" << capacity << "\n";
    std::cout << "warmup=" << warmup << "\n";
    std::cout << "measured_messages=" << measured_messages << "\n";
    std::cout << "elapsed_ns=" << elapsed_ns << "\n";
    std::cout << "throughput_msg_per_sec=" << throughput << "\n";
    std::cout << "hist_unit=ns bucket_ns=" << kBucketNs << " max_ns=" << kMaxNs
              << "\n";

    for (std::uint64_t consumer_id = 0; consumer_id < consumers;
         ++consumer_id) {
      std::cout << "consumer=" << consumer_id << "\n";
      std::cout << hists[consumer_id].report();
    }
  }
  return 0;
}
