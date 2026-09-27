#include "OwnNativeEllipsePlayer.hpp"
#include "PlayerSource.hpp"

#include <utility>

namespace {
using namespace avemotion;
using Playback = render::detail::OwnNativeEllipsePlayback;

// Static identity is part of the private typed-lookup contract. It also keeps
// the erased Player entry to its existing owner and one table pointer.
const player::detail::PlayerSourceOps kOwnOps{
    .frameRate = [](const void* object) noexcept {
        return static_cast<const Playback*>(object)->frameRate();
    },
    .snapshot = [](const void* object, runtime::MotionTime time) noexcept {
        return static_cast<const Playback*>(object)->playbackSnapshot(time);
    },
    .play = [](void* object, runtime::MotionTime time) noexcept {
        static_cast<Playback*>(object)->play(time);
    },
    .pause = [](void* object, runtime::MotionTime time) noexcept {
        static_cast<Playback*>(object)->pause(time);
    },
    .resume = [](void* object, runtime::MotionTime time) noexcept {
        static_cast<Playback*>(object)->resume(time);
    },
    .stop = [](void* object) noexcept {
        static_cast<Playback*>(object)->stop();
    },
    .seekNormalized = [](void* object, double position,
                         runtime::MotionTime time) noexcept {
        static_cast<Playback*>(object)->seekNormalized(position, time);
    },
    .setControlledProgress = [](void* object, double position) noexcept {
        static_cast<Playback*>(object)->setControlledProgress(position);
    },
    .setDirection = [](void* object, runtime::PlaybackDirection direction,
                       runtime::MotionTime time) noexcept {
        static_cast<Playback*>(object)->setDirection(direction, time);
    },
    .setPlaybackRate = [](void* object, double rate,
                          runtime::MotionTime time) noexcept {
        return static_cast<Playback*>(object)->setPlaybackRate(rate, time);
    },
    .setLoopMode = [](void* object, runtime::PlaybackLoopMode mode) noexcept {
        static_cast<Playback*>(object)->setLoopMode(mode);
    },
};
} // namespace

namespace avemotion::render::detail {

player::PlayerAddResult addOwnNativeEllipsePlayback(
    player::Player& player, std::shared_ptr<OwnNativeEllipsePlayback> playback,
    runtime::MotionTime now, player::PlayerEntryOptions options) {
    player::detail::PlayerSource source{
        .owner = std::move(playback),
        .ops = &kOwnOps,
        .runtimeInstance = nullptr,
        .runtimeHandle = {},
    };
    return player::detail::PlayerSourceAccess::add(
        player, std::move(source), now, options);
}

std::shared_ptr<OwnNativeEllipsePlayback> ownNativeEllipsePlayback(
    const player::Player& player, player::PlayerHandle handle) noexcept {
    auto source = player::detail::PlayerSourceAccess::source(player, handle);
    if (source.ops != &kOwnOps || !source.owner) return {};
    auto* typed = static_cast<OwnNativeEllipsePlayback*>(source.owner.get());
    return std::shared_ptr<OwnNativeEllipsePlayback>{
        std::move(source.owner), typed};
}

} // namespace avemotion::render::detail
