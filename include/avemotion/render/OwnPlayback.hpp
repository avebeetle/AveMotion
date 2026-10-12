#pragma once

#include "avemotion/player/Player.hpp"
#include "avemotion/runtime/EvaluatedScene.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <string>

namespace avemotion::render {

// Supported own profile only. Source bytes, preparation errors and scene
// ownership do not depend on a Runtime reference engine. This is a source
// interface; no stable binary ABI or redistribution guarantee is implied.

enum class OwnErrorCode : std::uint8_t {
    None, UnknownFormat, DecodeFailed, InvalidJson, ResourceLimit,
    UnsupportedAsset, PreparationFailed, InvalidAsset, IdentityExhausted,
    InvalidViewport, UnsupportedFrame, EvaluationFailed
};

struct OwnError final {
    OwnErrorCode code = OwnErrorCode::None;
    std::string path;
    std::string message;
    [[nodiscard]] explicit operator bool() const noexcept { return code != OwnErrorCode::None; }
};

struct OwnMetadata final {
    std::size_t logicalWidth = 0;
    std::size_t logicalHeight = 0;
    std::size_t totalFrames = 0;
    double frameRate = 0;
};
struct OwnTargetSize final { std::size_t width = 0, height = 0; };

struct OwnPlaybackResult;
struct OwnPrepareResult;
class OwnAsset;
OwnPlaybackResult createOwnPlayback(std::shared_ptr<const OwnAsset> asset);
OwnPrepareResult prepareOwnAsset(std::span<const std::byte> bytes);

class OwnAsset final {
public:
    ~OwnAsset();
    OwnAsset(const OwnAsset&) = delete;
    OwnAsset& operator=(const OwnAsset&) = delete;
    [[nodiscard]] const OwnMetadata& metadata() const noexcept;
private:
    struct Impl;
    friend OwnPrepareResult prepareOwnAsset(std::span<const std::byte>);
    friend OwnPlaybackResult createOwnPlayback(std::shared_ptr<const OwnAsset>);
    friend std::optional<OwnTargetSize> fitOwnTarget(
        const OwnAsset&, std::size_t, std::size_t) noexcept;
    explicit OwnAsset(std::unique_ptr<Impl> impl);
    std::unique_ptr<Impl> impl_;
};

struct OwnPrepareResult final {
    std::shared_ptr<const OwnAsset> asset;
    OwnError error;
    [[nodiscard]] explicit operator bool() const noexcept { return asset != nullptr && !error; }
};

[[nodiscard]] OwnPrepareResult prepareOwnAsset(std::span<const std::byte> bytes);

// Primitive assets use the requested box. Vector assets preserve the authored
// aspect ratio with integral fitted dimensions. An impossible positive target
// (or a zero box) yields no size. The caller owns any letterbox placement.
[[nodiscard]] std::optional<OwnTargetSize> fitOwnTarget(
    const OwnAsset& asset, std::size_t boxWidth, std::size_t boxHeight) noexcept;

class OwnPlayback final {
public:
    // Controls and evaluation are single-thread, serial operations. A scene
    // returned by evaluateAt owns its render data independently of playback.
    ~OwnPlayback();
    OwnPlayback(const OwnPlayback&) = delete;
    OwnPlayback& operator=(const OwnPlayback&) = delete;
    [[nodiscard]] const OwnAsset& asset() const noexcept;
    void play(runtime::MotionTime now) noexcept;
    void pause(runtime::MotionTime now) noexcept;
    void resume(runtime::MotionTime now) noexcept;
    void stop() noexcept;
    void seekNormalized(double position, runtime::MotionTime now) noexcept;
    void setControlledProgress(double position) noexcept;
    void setDirection(runtime::PlaybackDirection direction, runtime::MotionTime now) noexcept;
    [[nodiscard]] bool setPlaybackRate(double rate, runtime::MotionTime now) noexcept;
    void setLoopMode(runtime::PlaybackLoopMode mode) noexcept;
    // A completed snapshot projects the requested terminal state; it does not
    // indicate that a terminal scene was emitted or commit its revision.
    [[nodiscard]] runtime::PlaybackSnapshot playbackSnapshot(runtime::MotionTime now) const noexcept;
    [[nodiscard]] double frameRate() const noexcept;
    [[nodiscard]] double durationSeconds() const noexcept;
    struct Evaluation final {
        std::optional<runtime::EvaluatedScene> scene;
        OwnError error;
        [[nodiscard]] explicit operator bool() const noexcept { return scene.has_value() && !error; }
    };
    // Failed evaluation leaves the revision uncommitted. A successful terminal
    // evaluation commits completion once, even if evaluated again at that time.
    [[nodiscard]] Evaluation evaluateAt(runtime::MotionTime now,
                                        std::size_t width, std::size_t height);
private:
    struct Impl;
    friend OwnPlaybackResult createOwnPlayback(std::shared_ptr<const OwnAsset>);
    explicit OwnPlayback(std::unique_ptr<Impl> impl);
    std::unique_ptr<Impl> impl_;
};

struct OwnPlaybackResult final {
    std::shared_ptr<OwnPlayback> playback;
    OwnError error;
    [[nodiscard]] explicit operator bool() const noexcept { return playback != nullptr && !error; }
};
[[nodiscard]] OwnPlaybackResult createOwnPlayback(std::shared_ptr<const OwnAsset> asset);
[[nodiscard]] player::PlayerAddResult addOwnPlayback(
    player::Player& player, std::shared_ptr<OwnPlayback> playback,
    runtime::MotionTime now, player::PlayerEntryOptions options = {});
[[nodiscard]] std::shared_ptr<OwnPlayback> ownPlayback(
    const player::Player& player, player::PlayerHandle handle) noexcept;
// Player retains the public wrapper until removal/clear/destruction. Own tick
// frames have a null Runtime instance and invalid Runtime handle; resolve their
// PlayerHandle with ownPlayback, then evaluate on the same control/render thread.
// A Completion reason requests the projected terminal frame, not proof of scene
// emission. If evaluation fails, Player does not requeue it automatically;
// explicitly call Player::invalidate(handle) to request another frame.

} // namespace avemotion::render
