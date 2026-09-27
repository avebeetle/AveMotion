#include "OwnNativeEllipsePlayback.hpp"

#include <algorithm>
#include <utility>

namespace avemotion::render::detail {

OwnNativeEllipsePlayback::OwnNativeEllipsePlayback(
    std::shared_ptr<const OwnNativeEllipsePreparedAsset> prepared,
    std::unique_ptr<OwnNativeEllipseStream> stream)
    : prepared_(std::move(prepared)), stream_(std::move(stream)) {}
OwnNativeEllipsePlayback::~OwnNativeEllipsePlayback() = default;

OwnNativeEllipsePlaybackCreateResult OwnNativeEllipsePlayback::create(
    std::shared_ptr<const OwnNativeEllipsePreparedAsset> prepared) {
    auto result = OwnNativeEllipseStream::create(prepared);
    if (!result) return {result.code, std::move(result.message), nullptr};
    return {OwnNativeEllipseCreateCode::Ready, {},
        std::unique_ptr<OwnNativeEllipsePlayback>(
            new OwnNativeEllipsePlayback(std::move(prepared), std::move(result.stream)))};
}

void OwnNativeEllipsePlayback::play(runtime::MotionTime now) noexcept {
    runtime::detail::playPlayback(control_, now);
}
void OwnNativeEllipsePlayback::pause(runtime::MotionTime now) noexcept {
    runtime::detail::pausePlayback(control_, durationSeconds(), now);
}
void OwnNativeEllipsePlayback::resume(runtime::MotionTime now) noexcept {
    runtime::detail::resumePlayback(control_, now);
}
void OwnNativeEllipsePlayback::stop() noexcept {
    runtime::detail::stopPlayback(control_);
}
void OwnNativeEllipsePlayback::seekNormalized(double position, runtime::MotionTime now) noexcept {
    runtime::detail::seekPlayback(control_, position, now);
}
void OwnNativeEllipsePlayback::setControlledProgress(double position) noexcept {
    runtime::detail::controlPlayback(control_, position);
}
void OwnNativeEllipsePlayback::setDirection(
    runtime::PlaybackDirection direction, runtime::MotionTime now) noexcept {
    runtime::detail::setPlaybackDirection(control_, direction, durationSeconds(), now);
}
bool OwnNativeEllipsePlayback::setPlaybackRate(double rate, runtime::MotionTime now) noexcept {
    return runtime::detail::setPlaybackRate(control_, rate, durationSeconds(), now);
}
void OwnNativeEllipsePlayback::setLoopMode(runtime::PlaybackLoopMode mode) noexcept {
    runtime::detail::setPlaybackLoopMode(control_, mode);
}
runtime::PlaybackSnapshot OwnNativeEllipsePlayback::playbackSnapshot(
    runtime::MotionTime now) const noexcept {
    auto snapshot = runtime::detail::snapshotPlayback(control_, durationSeconds(), now);
    snapshot.frameIndex = frameAtPosition(snapshot.normalizedPosition);
    return snapshot;
}
std::size_t OwnNativeEllipsePlayback::frameAtPosition(double position) const noexcept {
    const auto count = prepared_->model->totalFrames;
    if (count <= 1U) return 0U;
    const double scaled = runtime::detail::clampPlaybackPosition(position)
        * static_cast<double>(count - 1U);
    return std::min(static_cast<std::size_t>(scaled), count - 1U);
}
double OwnNativeEllipsePlayback::durationSeconds() const noexcept {
    return static_cast<double>(static_cast<float>(prepared_->model->totalFrames - 1U)
        / static_cast<float>(prepared_->model->frameRate));
}
double OwnNativeEllipsePlayback::frameRate() const noexcept {
    return static_cast<double>(static_cast<float>(prepared_->model->frameRate));
}
OwnNativeEllipseFrameResult OwnNativeEllipsePlayback::evaluateAt(
    runtime::MotionTime now, std::size_t width, std::size_t height) {
    const auto snapshot = playbackSnapshot(now);
    auto result = stream_->emit(snapshot.frameIndex, width, height);
    if (result) runtime::detail::commitPlaybackCompletion(control_, snapshot, now);
    return result;
}

} // namespace avemotion::render::detail
