#pragma once
#include <cstddef>
#include <stdexcept>

namespace ull::detail {

inline bool is_power_of_two(std::size_t x) noexcept {
  return x != 0 && (x & (x - 1)) == 0;
}

inline std::size_t validate_capacity(std::size_t x) {
  if (!is_power_of_two(x)) {
    throw std::invalid_argument("Ring capacity must be a power of two");
  }
  return x;
}

} // namespace ull::detail
