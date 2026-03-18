#include <cassert>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <thread>

#include "ull/core/blocking_queue.h"

struct Msg {
  std::uint32_t seq;
  std::uint32_t payload;
};

int main() {
  // Constructor contract.
  {
    bool threw = false;
    try {
      ull::BlockingQueue<Msg> bad(0);
    } catch (const std::invalid_argument &) {
      threw = true;
    }
    assert(threw);
  }

  // Basic push/pop
  {
    ull::BlockingQueue<Msg> q(4);
    Msg m{1, 111};
    q.push(m);

    Msg out{};
    const bool ok = q.pop(out);
    assert(ok);
    assert(out.seq == 1);
    assert(out.payload == 111);
  }

  // FIFO behavior
  {
    ull::BlockingQueue<Msg> q(4);
    for (std::uint32_t i = 1; i <= 4; i++) {
      q.push(Msg{i, i * 10});
    }
    for (std::uint32_t i = 1; i <= 4; i++) {
      Msg out{};
      const bool ok = q.pop(out);
      assert(ok);
      assert(out.seq == i);
      assert(out.payload == i * 10);
    }
  }

  // close() causes pop() on empty queue to return false.
  {
    ull::BlockingQueue<Msg> q(4);
    q.close();

    Msg out{};
    const bool ok = q.pop(out);
    assert(!ok);
  }

  // push after close() should throw.
  {
    ull::BlockingQueue<Msg> q(4);
    q.close();
    bool threw = false;
    try {
      q.push(Msg{1, 111});
    } catch (const std::runtime_error &) {
      threw = true;
    }
    assert(threw);
  }

  // Consumer should wake up after producer pushes.
  {
    ull::BlockingQueue<Msg> q(4);
    Msg out{};
    std::thread producer([&] { q.push(Msg{42, 420}); });

    const bool ok = q.pop(out);
    assert(ok);
    assert(out.seq == 42);
    assert(out.payload == 420);

    producer.join();
  }

  std::cout << "test_blocking_queue PASS\n ";
  return 0;
}
