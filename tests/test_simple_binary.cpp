#include <cassert>
#include <cstdint>
#include <cstring>
#include <iostream>

#include "ull/proto/simple_binary.h"

int main() {
  ull::proto::Msg m{};
  m.tsc_send = 12345;
  m.seq = 7;
  m.msg_type = 1;
  m.payload = 0xaabbccdd;
  m.reserved = 0;

  // Serialize to raw buffer
  std::uint8_t buf[sizeof(m)];
  std::memcpy(buf, &m, sizeof(m));

  // Deserialize
  ull::proto::Msg out{};
  std::memcpy(&out, buf, sizeof(out));

  assert(out.tsc_send == m.tsc_send);
  assert(out.seq == m.seq);
  assert(out.msg_type == m.msg_type);
  assert(out.payload == m.payload);

  std::cout << "test_simple_binary PASS\n";

  return 0;
}
