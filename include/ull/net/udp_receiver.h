#pragma once
#include <cstddef>
#include <cstdint>

namespace ull::net {
class UdpReceiver {
public:
  explicit UdpReceiver(std::uint16_t port);
  ~UdpReceiver();

  UdpReceiver(const UdpReceiver &) = delete;
  UdpReceiver &operator=(const UdpReceiver &) = delete;

  UdpReceiver(UdpReceiver &&) = delete;
  UdpReceiver &operator=(UdpReceiver &&) = delete;

  // Blocking receive of one UDP datagram
  // Returns the number of bytes written into buf.
  std::size_t recv(void *buf, std::size_t len);

  int fd() const noexcept { return fd_; }

private:
  int fd_{-1};
};
} // namespace ull::net
