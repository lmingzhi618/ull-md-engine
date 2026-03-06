#include <cassert>
#include <cstdint>
#include <iostream>
#include <stdexcept>

#include "ull/core/spsc_ring.h"

struct Msg {
  std::uint32_t seq;
  std::uint32_t payload;
};

int main() {
  // Constructor contract: capacity is reported correctly.
  ull::SpscRing<Msg> q(8);
  assert(q.capacity() == 8);

  // Constructor contract: non-power-of-two capacity must fail.
  bool threw = false;
  try {
    ull::SpscRing<Msg> bad(7);
  } catch (const std::invalid_argument &) {
    threw = true;
  }
  assert(threw);

  // Empty pop
  Msg out{};
  assert(!q.try_pop(out));

  // Push then pop preserves payload
  Msg m1{1, 111};
  assert(q.try_push(m1));
  assert(q.try_pop(out));
  assert(out.seq == 1 && out.payload == 111);

  // fill to capacity: for cap = 8, we should be able to push 8 items,
  // and then the 9th push must fail.
  for (std::uint32_t i = 1; i <= 8; i++) {
    Msg m{i, i * 10};
    assert(q.try_push(m));
  }
  Msg extra{9, 90};
  assert(!q.try_push(extra));

  // drain and verify FIFO
  for (std::int32_t i = 1; i <= 8; i++) {
    assert(q.try_pop(out));
    assert(out.seq == i);
    assert(out.payload == i * 10);
  }
  assert(!q.try_pop(out));

  // Wrap-around stress: push/pop many times
  for (std::uint32_t i = 0; i < 10000; i++) {
    Msg m{i, i ^ 0xA5A5u};
    assert(q.try_push(m));
    assert(q.try_pop(out));
    assert(out.seq == i and out.payload == (i ^ 0xA5A5u));
  }

  std::cout << "test_spsc_ring PASS\n";
  return 0;
}
