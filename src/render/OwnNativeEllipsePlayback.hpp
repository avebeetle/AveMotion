#pragma once

#include "OwnNativeEllipseStream.hpp"
#include "PlaybackControl.hpp"

#include <memory>
#include <string>

namespace avemotion::render::detail {

class OwnNativeEllipsePlayback;

struct OwnNativeEllipsePlaybackCreateResult final {
    OwnNativeEllipseCreateCode code = OwnNativeEllipseCreateCode::InvalidPreparedAsset;
    std::string message;
    std::unique_ptr<OwnNativeEllipsePlayback> playback;
    [[nodiscard]] explicit operator bool() const noexcept {
        return code == OwnNativeEllipseCreateCode::Ready && playback != nullptr;
    }
};

// Own-only H domain: use the linked identity allocator with its module lifetime.
// Each playback is single-writer, including controls and evaluation. Callers
// retire scenes/plans before allocator unload and manage planner/backend forget
// or reset; destroying playback does not evict externally owned caches.
class OwnNativeEllipsePlayback final {
public:
    [[nodiscard]] static OwnNativeEllipsePlaybackCreateResult create(
        std::shared_ptr<const OwnNativeEllipsePreparedAsset> prepared);
    OwnNativeEllipsePlayback(const OwnNativeEllipsePlayback&) = delete;
    OwnNativeEllipsePlayback& operator=(const OwnNativeEllipsePlayback&) = delete;
    OwnNativeEllipsePlayback(OwnNativeEllipsePlayback&&) = delete;
    OwnNativeEllipsePlayback& operator=(OwnNativeEllipsePlayback&&) = delete;
    ~OwnNativeEllipsePlayback();

    void play(runtime::MotionTime now) noexcept;
    void pause(runtime::MotionTime now) noexcept;
    void resume(runtime::MotionTime now) noexcept;
    void stop() noexcept;
    void seekNormalized(double position, runtime::MotionTime now) noexcept;
    void setControlledProgress(double position) noexcept;
    void setDirection(runtime::PlaybackDirection direction, runtime::MotionTime now) noexcept;
    [[nodiscard]] bool setPlaybackRate(double rate, runtime::MotionTime now) noexcept;
    void setLoopMode(runtime::PlaybackLoopMode mode) noexcept;
    [[nodiscard]] runtime::PlaybackSnapshot playbackSnapshot(runtime::MotionTime now) const noexcept;
    [[nodiscard]] std::size_t frameAtPosition(double position) const noexcept;
    [[nodiscard]] double durationSeconds() const noexcept;
    [[nodiscard]] double frameRate() const noexcept;
    [[nodiscard]] const OwnNativeEllipsePreparedAsset& preparedAsset() const noexcept;
    [[nodiscard]] OwnNativeEllipseFrameResult evaluateAt(
        runtime::MotionTime now, std::size_t width, std::size_t height);

private:
    OwnNativeEllipsePlayback(std::shared_ptr<const OwnNativeEllipsePreparedAsset> prepared,
                             std::unique_ptr<OwnNativeEllipseStream> stream);
    std::shared_ptr<const OwnNativeEllipsePreparedAsset> prepared_;
    std::unique_ptr<OwnNativeEllipseStream> stream_;
    runtime::detail::PlaybackControl control_;
};

} // namespace avemotion::render::detail
