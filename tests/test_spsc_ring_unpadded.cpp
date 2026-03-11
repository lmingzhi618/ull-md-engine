#include "ull/core/spsc_ring_unpadded.h"
#include <cassert>
#include <cstdint>
#include <iostream>
#include <stdexcept>

struct Msg {
  std::uint32_t seq;
  std::uint32_t payload;
};

int main() {
  ull::SpscRingUnpadded<Msg> q(8);
  assert(q.capacity() == 8);
  bool threw = false;
  try {
    ull::SpscRingUnpadded<Msg> bad(7);
  } catch (const std::invalid_argument &) {
    threw = true;
  }
  assert(threw);

  Msg out{};
  assert(!q.try_pop(out));

  Msg m1{1, 111};
  assert(q.try_push(m1));
  assert(q.try_pop(out));
  assert(out.seq == 1);
  assert(out.payload == 111);

  for (std::uint32_t i = 1; i <= 8; i++) {
    Msg m{i, i * 10};
    assert(q.try_push(m));
  }

  Msg extra{9, 90};
  assert(!q.try_push(extra));

  for (std::uint32_t i = 1; i <= 8; i++) {
    assert(q.try_pop(out));
    assert(out.seq == i);
    assert(out.payload = i * 10);
  }
  assert(!q.try_pop(out));

  for (std::uint32_t i = 0; i < 10000; i++) {
    Msg m{i, i ^ 0xA5A5u};
    assert(q.try_push(m));
    assert(q.try_pop(out));
    assert(out.seq == i);
    assert(out.payload == (i ^ 0xA5A5u));
  }
  std::cout << "test_spsc_ring_unpadded PASS\n";
  return 0;
}
