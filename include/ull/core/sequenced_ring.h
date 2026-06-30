#pragma once

#include <cstdint>
#include <vector>

#include "ull/core/utils.h"

namespace ull {

// SequencedRing owns payload storage only.
// It does not decide whether a sequence is claimed, published, or safe to
// reuse. Those rules are owned by the sequencer/barrier/gating logic.
template <typename T> class SequencedRing {
public:
  explicit SequencedRing(std::uint64_t capacity)
      : capacity_(detail::validate_capacity(capacity)), mask_(capacity_ - 1),
        storage_(capacity_) {}

  std::uint64_t capacity() const noexcept { return capacity_; }

  std::uint64_t index(std::uint64_t seq) const noexcept { return seq & mask_; }

  void write(std::uint64_t seq, const T &value) {
    storage_[index(seq)] = value;
  }

  T &read(std::uint64_t seq) noexcept { return storage_[index(seq)]; }

  const T &read(std::uint64_t seq) const noexcept {
    return storage_[index(seq)];
  }

private:
  std::uint64_t capacity_;
  std::uint64_t mask_;
  std::vector<T> storage_;
};
} // namespace ull
