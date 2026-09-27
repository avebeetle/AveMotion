#include "PlaybackControl.hpp"

#include <algorithm>
#include <cmath>

namespace avemotion::runtime::detail {
namespace {

[[nodiscard]] double positiveUnitModulo(double value) noexcept {
    if (!std::isfinite(value)) return 0.0;
    const auto result = value - std::floor(value);
    return result >= 1.0 ? 0.0 : (result < 0.0 ? result + 1.0 : result);
}

struct ResolvedPlayback final {
    double position = 0.0;
    bool completed = false;
};

[[nodiscard]] ResolvedPlayback resolvePlayback(
    const PlaybackControl& control, double duration,
    MotionTime presentationTime) noexcept {
    if (control.status != PlaybackStatus::Playing) {
        return {
            clampPlaybackPosition(control.anchorPosition),
            control.status == PlaybackStatus::Holding};
    }
    if (!std::isfinite(duration) || duration <= 0.0
        || !std::isfinite(control.playbackRate)
        || control.playbackRate <= 0.0) {
        return {clampPlaybackPosition(control.anchorPosition), true};
    }
    const long double deltaNanoseconds =
        static_cast<long double>(presentationTime.nanoseconds)
        - static_cast<long double>(control.anchorTime.nanoseconds);
    const long double elapsedSeconds = deltaNanoseconds / 1'000'000'000.0L;
    const auto sign = control.direction == PlaybackDirection::Forward
        ? 1.0L : -1.0L;
    const long double raw = static_cast<long double>(control.anchorPosition)
        + sign * elapsedSeconds
            * static_cast<long double>(control.playbackRate)
            / static_cast<long double>(duration);
    const auto rawDouble = static_cast<double>(raw);
    if (control.loopMode == PlaybackLoopMode::Loop) {
        // Reverse playback authored from the terminal endpoint must expose the
        // last sample at its exact anchor time before wrapping on later ticks.
        if (control.direction == PlaybackDirection::Reverse
            && control.anchorPosition == 1.0
            && presentationTime == control.anchorTime) {
            return {1.0, false};
        }
        return {positiveUnitModulo(rawDouble), false};
    }
    if (control.direction == PlaybackDirection::Forward) {
        return {clampPlaybackPosition(rawDouble), rawDouble >= 1.0};
    }
    return {clampPlaybackPosition(rawDouble), rawDouble <= 0.0};
}

void reanchor(PlaybackControl& control, double duration,
    MotionTime presentationTime) noexcept {
    const auto resolved = resolvePlayback(control, duration, presentationTime);
    control.anchorPosition = resolved.position;
    control.anchorTime = presentationTime;
}

} // namespace

double clampPlaybackPosition(double value) noexcept {
    return std::isfinite(value) ? std::clamp(value, 0.0, 1.0) : 0.0;
}

void playPlayback(PlaybackControl& control, MotionTime now) noexcept {
    if (control.status == PlaybackStatus::Playing) return;
    if (control.status == PlaybackStatus::Holding) {
        control.anchorPosition =
            control.direction == PlaybackDirection::Forward ? 0.0 : 1.0;
    }
    control.anchorTime = now;
    control.status = PlaybackStatus::Playing;
    ++control.revision;
}

void pausePlayback(PlaybackControl& control, double duration,
    MotionTime now) noexcept {
    if (control.status != PlaybackStatus::Playing) return;
    reanchor(control, duration, now);
    control.status = PlaybackStatus::Paused;
    ++control.revision;
}

void resumePlayback(PlaybackControl& control, MotionTime now) noexcept {
    if (control.status == PlaybackStatus::Playing) return;
    control.anchorTime = now;
    control.status = PlaybackStatus::Playing;
    ++control.revision;
}

void stopPlayback(PlaybackControl& control) noexcept {
    control.status = PlaybackStatus::Stopped;
    control.anchorPosition =
        control.direction == PlaybackDirection::Forward ? 0.0 : 1.0;
    ++control.revision;
}

void seekPlayback(PlaybackControl& control, double position,
    MotionTime now) noexcept {
    control.anchorPosition = clampPlaybackPosition(position);
    control.anchorTime = now;
    if (control.status == PlaybackStatus::Holding) {
        control.status = PlaybackStatus::Paused;
    }
    ++control.revision;
}

void controlPlayback(PlaybackControl& control, double position) noexcept {
    control.anchorPosition = clampPlaybackPosition(position);
    control.status = PlaybackStatus::Controlled;
    ++control.revision;
}

void setPlaybackDirection(PlaybackControl& control,
    PlaybackDirection direction, double duration, MotionTime now) noexcept {
    if (control.direction == direction) return;
    if (control.status == PlaybackStatus::Stopped) {
        control.anchorPosition =
            direction == PlaybackDirection::Forward ? 0.0 : 1.0;
        control.anchorTime = now;
    } else {
        reanchor(control, duration, now);
    }
    control.direction = direction;
    ++control.revision;
}

bool setPlaybackRate(PlaybackControl& control, double rate,
    double duration, MotionTime now) noexcept {
    if (!std::isfinite(rate) || rate <= 0.0) return false;
    reanchor(control, duration, now);
    control.playbackRate = rate;
    ++control.revision;
    return true;
}

void setPlaybackLoopMode(PlaybackControl& control,
    PlaybackLoopMode mode) noexcept {
    control.loopMode = mode;
    ++control.revision;
}

PlaybackSnapshot snapshotPlayback(const PlaybackControl& control,
    double duration, MotionTime now) noexcept {
    PlaybackSnapshot snapshot;
    snapshot.status = control.status;
    snapshot.direction = control.direction;
    snapshot.loopMode = control.loopMode;
    snapshot.clip = control.clip;
    snapshot.playbackRate = control.playbackRate;
    snapshot.revision = control.revision;
    const auto resolved = resolvePlayback(control, duration, now);
    snapshot.normalizedPosition = resolved.position;
    snapshot.completed = resolved.completed;
    if (resolved.completed && snapshot.status == PlaybackStatus::Playing) {
        snapshot.status = PlaybackStatus::Holding;
    }
    return snapshot;
}

void commitPlaybackCompletion(PlaybackControl& control,
    const PlaybackSnapshot& snapshot, MotionTime now) noexcept {
    if (snapshot.completed
        && control.status == PlaybackStatus::Playing
        && control.loopMode == PlaybackLoopMode::Once) {
        control.anchorPosition = snapshot.normalizedPosition;
        control.anchorTime = now;
        control.status = PlaybackStatus::Holding;
        ++control.revision;
    }
}

} // namespace avemotion::runtime::detail
