#include "ull/net/udp_receiver.h"
#include <arpa/inet.h>
#include <cerrno>
#include <cstring>
#include <stdexcept>
#include <string>
#include <sys/socket.h>
#include <unistd.h>

namespace ull::net {

namespace {
std::runtime_error sys_error(const std::string &what) {
  return std::runtime_error(what + ": " + std::strerror(errno));
}
} // namespace

UdpReceiver::UdpReceiver(std::uint16_t port) {
  fd_ = ::socket(AF_INET, SOCK_DGRAM, 0);
  if (fd_ < 0) {
    throw sys_error("socket(AF_INET, SOCK_DGRAM) failed");
  }

  sockaddr_in addr{};
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

  if (::bind(fd_, reinterpret_cast<const sockaddr *>(&addr), sizeof(addr)) <
      0) {
    ::close(fd_);
    fd_ = -1;
    throw sys_error("bind() failed");
  }
}

UdpReceiver::~UdpReceiver() {
  if (fd_ >= 0) {
    ::close(fd_);
    fd_ = -1;
  }
}

std::size_t UdpReceiver::recv(void *buf, std::size_t len) {
  const auto n = ::recvfrom(fd_, buf, len, 0, nullptr, nullptr);
  if (n < 0) {
    throw sys_error("recvfrom() failed");
  }
  return static_cast<std::size_t>(n);
}
} // namespace ull::net
