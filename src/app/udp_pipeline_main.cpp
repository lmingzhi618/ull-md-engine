#include <arpa/inet.h>
#include <atomic>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>

#include "ull/core/spsc_ring.h"
#include "ull/net/udp_receiver.h"
#include "ull/perf/latency_hist.h"
#include "ull/perf/ticks.h"
#include "ull/proto/simple_binary.h"

int main(int argc, char **argv) {
  const std::uint32_t N =
      (argc >= 2) ? static_cast<std::uint32_t>(std::stoul(argv[1])) : 2'000'000;
  const std::uint32_t WARMUP =
      (argc >= 3) ? static_cast<std::uint32_t>(std::stoul(argv[2])) : 200'000;
  const std::uint16_t PORT =
      (argc >= 4) ? static_cast<std::uint16_t>(std::stoul(argv[3])) : 19001;

  ull::perf::init_ticks();

  ull::net::UdpReceiver receiver(PORT);
  ull::SpscRing<ull::proto::Msg> q(1u << 16);

  constexpr std::uint64_t kMaxNs = 5'000'000;
  constexpr std::uint64_t kBucketNs = 50;
  ull::perf::LatencyHist hist(kMaxNs, kBucketNs);

  std::atomic<bool> done{false};

  // Consumer thread: pop messages frmo the queue and record latency.
  std::thread consumer([&] {
    ull::proto::Msg m{};
    std::uint64_t seen = 0;

    while (!done.load(std::memory_order_acquire)) {
      while (q.try_pop(m)) {
        const auto t1 = ull::perf::ticks();
        const auto dt_ns = ull::perf::ticks_to_ns(t1 - m.tsc_send);

        ++seen;
        if (seen > WARMUP) {
          hist.add(dt_ns);
        }
      }
    }

    // Drain remaining messages after receiver is done.
    while (q.try_pop(m)) {
      const auto t1 = ull::perf::ticks();
      const auto dt_ns = ull::perf::ticks_to_ns(t1 - m.tsc_send);

      ++seen;
      if (seen > WARMUP) {
        hist.add(dt_ns);
      }
    }
  });

  // Sender thread: send N UDP datagrams to lookback.
  std::thread sender([&] {
    int fd = ::socket(AF_INET, SOCK_DGRAM, 0);
    assert(fd >= 0);

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    for (std::uint32_t i = 1; i <= N; i++) {
      ull::proto::Msg msg{};
      msg.tsc_send = ull::perf::ticks();
      msg.seq = i;
      msg.msg_type = 1;
      msg.payload = static_cast<std::uint64_t>(i) ^ 0xA5A5A5A5ULL;
      msg.reserved = 0;

      const auto n =
          ::sendto(fd, &msg, sizeof(msg), 0,
                   reinterpret_cast<const sockaddr *>(&addr), sizeof(addr));
      assert(n == static_cast<ssize_t>(sizeof(msg)));
    }
    ::close(fd);
  });

  // Receiver loop on main thread: recv UDP bytes, parse Msg, enqueue.
  std::uint32_t received = 0;
  while (received < N) {
    std::uint8_t buf[sizeof(ull::proto::Msg)]{};
    const auto n = receiver.recv(buf, sizeof(buf));

    if (n != sizeof(ull::proto::Msg)) {
      std::cerr << "unexpected datagram size: " << n << " bytes, expected "
                << sizeof(ull::proto::Msg) << "\n";
      return 1;
    }

    ull::proto::Msg msg{};
    std::memcpy(&msg, buf, sizeof(msg));
    while (!q.try_push(msg)) {
      // MVP: spin until queue has space
    }
    ++received;
  }

  sender.join();
  done.store(true, std::memory_order_release);
  consumer.join();

  std::cout << "N=" << N << " warmup=" << WARMUP << " port=" << PORT << "\n";
  std::cout << "hist_unit=ms bucket_ns=" << kBucketNs << " max_ns=" << kMaxNs
            << "\n";
  std::cout << hist.report();
  return 0;
}
