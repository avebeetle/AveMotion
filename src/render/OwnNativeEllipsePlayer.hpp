#pragma once

#include "OwnNativeEllipsePlayback.hpp"
#include "avemotion/player/Player.hpp"

#include <memory>

namespace avemotion::render::detail {

// Private own-source binding. Player tick views borrow their frame span until
// the next non-const Player call. Own ScheduledFrame entries have a PlayerHandle
// for scheduling, but their Runtime instance pointer is null and their Runtime
// InstanceHandle is invalid. They must be resolved through the typed lookup;
// own scene.instanceId is a separate planner/cache identity and must never be
// formed by casting either handle. The host evaluates own scenes after tick;
// failed evaluation needs explicit Player invalidation for a retry.
//
// Registration retains the playback until removal, clear, or Player destruction.
// Those operations release scheduling ownership, not scenes/plans or graphics
// caches: the caller retires own scene IDs with planner forget/reset and backend
// clear/domain invalidation. Own and Runtime entries may share this Player but
// must never share a planner/backend graphics domain. Calls to Player and each
// playback remain serial, callbacks must not re-enter Player, and the linked
// own identity allocator must outlive retained own objects/scenes/plans.
[[nodiscard]] player::PlayerAddResult addOwnNativeEllipsePlayback(
    player::Player& player,
    std::shared_ptr<OwnNativeEllipsePlayback> playback,
    runtime::MotionTime now,
    player::PlayerEntryOptions options = {});

[[nodiscard]] std::shared_ptr<OwnNativeEllipsePlayback> ownNativeEllipsePlayback(
    const player::Player& player,
    player::PlayerHandle handle) noexcept;

} // namespace avemotion::render::detail
