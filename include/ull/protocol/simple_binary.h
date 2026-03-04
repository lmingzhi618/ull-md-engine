#pragma once
#include <cstdint>

namespace ull::proto {

// MVP message (placeholder for real market data):
// - tsc_send: producer timestamp
// - seq: monotonically increasing sequence number
// - payload: dummy data
struct alignas(16) Msg {
  std::uint64_t tsc_send;
  std::uint32_t seq;
  std::uint32_t payload;
};
} // namespace ull::proto
