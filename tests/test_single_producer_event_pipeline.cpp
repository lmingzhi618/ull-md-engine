#include <cassert>
#include <iostream>

#include "ull/core/single_producer_event_pipeline.h"

int main() {
  {
    ull::SingleProducerEventPipeline<std::uint64_t> p(4);

    const auto seq = p.next();

    p.write(seq, 100);

    p.publish(seq);

    const auto available = p.wait_until_available(seq);
    assert(available >= seq);
    assert(p.read(seq) == 100);

    p.mark_consumed(seq);
  }
  {
    ull::SingleProducerEventPipeline<std::uint64_t> p(4);

    for (std::uint64_t i = 0; i < 4; ++i) {
      const auto seq = p.next();
      p.write(seq, i);
      p.publish(seq);

      p.wait_until_available(seq);
      p.mark_consumed(seq);
    }
    const auto seq = p.next();
    assert(seq == 4);
    p.write(seq, 500);
    p.publish(seq);
    assert(p.read(4) == 500);
    assert(p.read(0) == 500);
  }
  {
    ull::SingleProducerEventPipeline<std::uint64_t> p(4);

    for (std::uint64_t i = 0; i < 4; ++i) {
      const auto seq = p.next();
      p.write(seq, i);
      p.publish(seq);
    }

    std::uint64_t seq;
    assert(p.remaining_capacity() == 0);
    assert(!p.try_next(seq));

    p.mark_consumed(0);

    assert(p.remaining_capacity() == 1);
    assert(p.try_next(seq));
    assert(seq == 4);
  }
  {
    ull::SpinWaitStrategy strategy(ull::util::SpinStrategy::ThreadYield);

    ull::SingleProducerEventPipeline<std::uint64_t, ull::SpinWaitStrategy> p(
        4, strategy);

    const auto seq = p.next();

    p.write(seq, 123);
    p.publish(seq);

    const auto available = p.wait_until_available(seq);
    assert(available >= seq);
    assert(p.read(seq) == 123);

    p.mark_consumed(seq);
  }
  std::cout << "test_single_producer_event_pipeline PASS\n";
  return 0;
}
