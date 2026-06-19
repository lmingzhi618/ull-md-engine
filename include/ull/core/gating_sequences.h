#pragma once

#include <cstdint>
#include <memory>
#include <vector>

#include "ull/core/sequence.h"

namespace ull {

class GatingSequences {
public:
  explicit GatingSequences(std::uint64_t count) {
    sequences_.reserve(count);

    for (std::uint64_t i = 0; i < count; ++i) {
      sequences_.emplace_back(std::make_unique<Sequence>(kNoConsumerProgress));
    }
  }
  std::uint64_t count() const noexcept {
    return static_cast<std::uint64_t>(sequences_.size());
  }
  std::uint64_t load_min() const noexcept {
    if (sequences_.empty()) {
      return kNoConsumerProgress;
    }

    auto min_seq = sequences_[0]->load();

    for (std::uint64_t i = 1; i < sequences_.size(); ++i) {
      const auto seq = sequences_[i]->load();
      if (seq < min_seq) {
        min_seq = seq;
      }
    }

    return min_seq;
  }

  void store(std::uint64_t index, std::uint64_t seq) noexcept {
    if (index >= sequences_.size()) {
      return;
    }
    sequences_[index]->store(seq, std::memory_order_release);
  }
  std::uint64_t load(std::uint64_t index) const noexcept {
    if (index >= sequences_.size()) {
      return kNoConsumerProgress;
    }
    return sequences_[index]->load();
  }

private:
  std::vector<std::unique_ptr<Sequence>> sequences_;
};
} // namespace ull
