#include <cassert>
#include <cstdint>
#include <iostream>
#include <thread>
#include <vector>

#include "ull/core/mpsc_ring.h"

struct Msg {
  std::uint32_t producer;
  std::uint32_t seq;
};

int main() {
  {
    ull::MpscRing<Msg> q(8);
    assert(q.capacity() == 8);

    Msg out{};
    assert(!q.try_pop(out));

    assert(q.try_push(Msg{1, 42}));
    assert(q.try_pop(out));
    assert(out.producer == 1);
    assert(out.seq == 42);
    assert(!q.try_pop(out));
  }
  {
    constexpr std::uint32_t kProducers = 4;
    constexpr std::uint32_t kPerProducer = 10000;
    constexpr std::uint32_t kTotal = kProducers * kPerProducer;

    ull::MpscRing<Msg> q(1u << 16);

    std::atomic<std::uint32_t> consumed{0};
    std::vector<std::uint32_t> counts(kProducers, 0);

    std::thread consumer([&] {
      Msg out{};
      while (consumed.load(std::memory_order_relaxed) < kTotal) {
        if (q.try_pop(out)) {
          assert(out.producer < kProducers);
          ++counts[out.producer];
          consumed.fetch_add(1, std::memory_order_relaxed);
        }
      }
    });

    std::vector<std::thread> producers;
    producers.reserve(kProducers);

    for (std::uint32_t p = 0; p < kProducers; ++p) {
      producers.emplace_back([&, p] {
        for (std::uint32_t i = 0; i < kPerProducer; ++i) {
          q.try_push(Msg{p, i});
        }
      });
    }

    for (auto &t : producers) {
      t.join();
    }

    consumer.join();

    assert(consumed.load() == kTotal);

    for (std::uint32_t p = 0; p < kProducers; ++p) {
      assert(counts[p] == kPerProducer);
    }
  }

  std::cout << "test_mpsc_ring PASS\n";
  return 0;
}
