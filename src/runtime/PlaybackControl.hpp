#pragma once

#include "avemotion/runtime/Playback.hpp"

namespace avemotion::runtime::detail {

struct PlaybackControl final {
    PlaybackStatus status = PlaybackStatus::Stopped;
    PlaybackDirection direction = PlaybackDirection::Forward;
    PlaybackLoopMode loopMode = PlaybackLoopMode::Loop;
    model::ClipId clip{0U};
    double playbackRate = 1.0;
    double anchorPosition = 0.0;
    MotionTime anchorTime;
    std::uint64_t revision = 1U;
};

[[nodiscard]] double clampPlaybackPosition(double value) noexcept;
void playPlayback(PlaybackControl& control, MotionTime now) noexcept;
void pausePlayback(PlaybackControl& control, double duration, MotionTime now) noexcept;
void resumePlayback(PlaybackControl& control, MotionTime now) noexcept;
void stopPlayback(PlaybackControl& control) noexcept;
void seekPlayback(PlaybackControl& control, double position, MotionTime now) noexcept;
void controlPlayback(PlaybackControl& control, double position) noexcept;
void setPlaybackDirection(PlaybackControl& control, PlaybackDirection direction,
    double duration, MotionTime now) noexcept;
[[nodiscard]] bool setPlaybackRate(PlaybackControl& control, double rate,
    double duration, MotionTime now) noexcept;
void setPlaybackLoopMode(PlaybackControl& control, PlaybackLoopMode mode) noexcept;
[[nodiscard]] PlaybackSnapshot snapshotPlayback(const PlaybackControl& control,
    double duration, MotionTime now) noexcept;
void commitPlaybackCompletion(PlaybackControl& control,
    const PlaybackSnapshot& snapshot, MotionTime now) noexcept;

} // namespace avemotion::runtime::detail
