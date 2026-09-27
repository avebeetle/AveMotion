#pragma once

#include <atomic>
#include <cstdint>
#include <optional>

namespace avemotion::render::detail {

// Production counter operations exposed privately for boundary tests. They do
// not mutate any live stream or the linked allocator's global counter.
[[nodiscard]] std::optional<std::uint64_t> tryNextOwnStreamIdentity(
    std::atomic<std::uint64_t>& last) noexcept;
[[nodiscard]] bool tryAdvanceOwnStreamSequence(std::uint64_t& last) noexcept;

} // namespace avemotion::render::detail
