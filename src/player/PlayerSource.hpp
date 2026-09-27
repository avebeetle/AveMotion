#pragma once

#include "avemotion/player/Player.hpp"

#include <memory>

namespace avemotion::player::detail {

struct PlayerSourceOps final {
    double (*frameRate)(const void*) noexcept = nullptr;
    runtime::PlaybackSnapshot (*snapshot)(const void*, runtime::MotionTime) noexcept = nullptr;
    void (*play)(void*, runtime::MotionTime) noexcept = nullptr;
    void (*pause)(void*, runtime::MotionTime) noexcept = nullptr;
    void (*resume)(void*, runtime::MotionTime) noexcept = nullptr;
    void (*stop)(void*) noexcept = nullptr;
    void (*seekNormalized)(void*, double, runtime::MotionTime) noexcept = nullptr;
    void (*setControlledProgress)(void*, double) noexcept = nullptr;
    void (*setDirection)(void*, runtime::PlaybackDirection, runtime::MotionTime) noexcept = nullptr;
    bool (*setPlaybackRate)(void*, double, runtime::MotionTime) noexcept = nullptr;
    void (*setLoopMode)(void*, runtime::PlaybackLoopMode) noexcept = nullptr;
};

struct PlayerSource final {
    std::shared_ptr<void> owner;
    const PlayerSourceOps* ops = nullptr;
    runtime::Instance* runtimeInstance = nullptr;
    runtime::InstanceHandle runtimeHandle;
};

struct PlayerSourceAccess final {
    [[nodiscard]] static PlayerAddResult add(
        Player& player,
        PlayerSource source,
        runtime::MotionTime now,
        PlayerEntryOptions options = {});

    [[nodiscard]] static PlayerSource source(
        const Player& player,
        PlayerHandle handle) noexcept;
};

} // namespace avemotion::player::detail
