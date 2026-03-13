#pragma once
#include <cstddef>
#include <new>

namespace ull {
inline constexpr std::size_t kCacheLine =
#ifdef __cpp_lib_hardware_interference_size
    std::hardware_destructive_interference_size;
#else
    64;
#endif
} // namespace ull
