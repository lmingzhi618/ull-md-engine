#include <cassert>
#include <cstdint>
#include <iostream>
#include <thread>

#include "ull/core/single_producer_event_pipeline.h"

int main() {
  {
    constexpr std::uint64_t capacity = 1024;
    constexpr std::uint64_t messages = 100000;

    ull::SingleProducerEventPipeline<std::uint64_t> pipeline(capacity);

    std::uint64_t consumed = 0;

    std::thread consumer([&] {
      for (std::uint64_t expected = 0; expected < messages; ++expected) {
        // Consumer visibility boundary:
        // do not read ring storage until producer has published this sequence.
        const auto available = pipeline.wait_until_available(expected);
        assert(available >= expected);

        const auto value = pipeline.read(expected);
        assert(value == expected);

        pipeline.mark_consumed(expected);
        ++consumed;
      }
    });

    std::thread producer([&] {
      for (std::uint64_t i = 0; i < messages; ++i) {
        // Producer lifecycle:
        // claim sequence -> write payload -> publish visibility.
        const auto seq = pipeline.next();
        assert(seq == i);

        pipeline.write(seq, i);
        pipeline.publish(seq);
      }
    });

    producer.join();
    consumer.join();

    assert(consumed == messages);
    assert(pipeline.remaining_capacity() == capacity);
  }

  std::cout << "test_single_producer_event_pipeline_threaded PASS\n";
  return 0;
}
