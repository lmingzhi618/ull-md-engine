#include <arpa/inet.h>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>

#include "ull/net/udp_receiver.h"
#include "ull/proto/simple_binary.h"

int main() {
  constexpr std::uint16_t kPort = 19001;
  ull::net::UdpReceiver receiver(kPort);

  std::thread sender([] {
    int fd = ::socket(AF_INET, SOCK_DGRAM, 0);
    assert(fd >= 0);

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(kPort);
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    ull::proto::Msg msg{};
    msg.tsc_send = 123456789;
    msg.seq = 42;
    msg.msg_type = 1;
    msg.payload = 0xABCDEFULL;
    msg.reserved = 0;

    const auto n =
        ::sendto(fd, &msg, sizeof(msg), 0,
                 reinterpret_cast<const sockaddr *>(&addr), sizeof(addr));
    assert(n == static_cast<ssize_t>(sizeof(msg)));

    ::close(fd);
  });

  std::uint8_t buf[sizeof(ull::proto::Msg)]{};
  const auto n = receiver.recv(buf, sizeof(buf));
  assert(n == sizeof(ull::proto::Msg));

  ull::proto::Msg out{};
  std::memcpy(&out, buf, sizeof(out));

  assert(out.tsc_send == 123456789);
  assert(out.seq == 42);
  assert(out.msg_type == 1);
  assert(out.payload = 0xABCDEFULL);
  assert(out.reserved == 0);

  sender.join();

  std::cout << "test_udp_receiver PASS\n";
  return 0;
}
