#include <atomic>
#include <chrono>
#include <iostream>
#include <thread>

constexpr std::uint64_t N = 200'000'000;

struct Unpadded {
  std::atomic<std::uint64_t> a{0};
  std::atomic<std::uint64_t> b{0};
};

struct Padded {
  alignas(64) std::atomic<std::uint64_t> a{0};
  alignas(64) std::atomic<std::uint64_t> b{0};
};

template <typename T> void run(const std::string &mode) {
  T data;

  auto start = std::chrono::steady_clock::now();

  std::thread t1([&] {
    for (std::uint64_t i = 0; i < N; i++) {
      data.a.fetch_add(1, std::memory_order_relaxed);
    }
  });

  std::thread t2([&] {
    for (std::uint64_t i = 0; i < N; i++) {
      data.b.fetch_add(1, std::memory_order_relaxed);
    }
  });

  t1.join();
  t2.join();

  auto end = std::chrono::steady_clock::now();
  auto ns =
      std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();

  std::cout << "mode=" << mode << "\n";
  std::cout << "time_ns=" << ns << "\n";
  std::cout << "ops_per_thread=" << N << "\n";
  std::cout << "total_ops=" << 2 * N << "\n";
}

int main(int argc, char **argv) {
  std::string mode = (argc >= 2) ? argv[1] : "padded";

  if (mode == "padded") {
    run<Padded>("padded");
    return 0;
  }

  if (mode == "unpadded") {
    run<Unpadded>("unpadded");
    return 0;
  }

  std::cerr << "unknown mode\n";
  return 1;
}
