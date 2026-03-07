#pragma once
#include <cstdint>

namespace ull::proto {

struct alignas(16) Msg {
  std::uint64_t tsc_send; // send timestamp (ticks)
  std::uint32_t seq;      // sequence number
  std::uint32_t msg_type; // application-defined type
  std::uint64_t payload;  // arbitrary payload
  std::uint64_t reserved; // reserved for future extensions
};

static_assert(sizeof(Msg) == 32, "Msg must be 32 bytes");
} // namespace ull::proto
