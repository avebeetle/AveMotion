#include "avemotion/render/OwnPlayback.hpp"
#include "OwnNativeEllipsePlayback.hpp"
#include "OwnNativeEllipsePreparedAsset.hpp"
#include "OwnJsonReader.hpp"
#include "PlayerSource.hpp"
#include "avemotion/formats/Tgs.hpp"

#include <cmath>
#include <string_view>
#include <utility>

namespace avemotion::render {
namespace {
constexpr std::size_t kMaximumJsonBytes = 1'048'576;
OwnError error(OwnErrorCode code, std::string path, std::string message) {
    return {code, std::move(path), std::move(message)};
}
}

struct OwnAsset::Impl final {
    std::shared_ptr<const detail::OwnNativeEllipsePreparedAsset> prepared;
    OwnMetadata metadata;
    bool vectorAuthored = false;
};
OwnAsset::OwnAsset(std::unique_ptr<Impl> impl) : impl_(std::move(impl)) {}
OwnAsset::~OwnAsset() = default;
const OwnMetadata& OwnAsset::metadata() const noexcept { return impl_->metadata; }

OwnPrepareResult prepareOwnAsset(std::span<const std::byte> bytes) {
    const auto format = formats::detectAssetFormat(bytes);
    std::string decoded;
    std::string_view json;
    if (format == formats::AssetFormat::TelegramTgs) {
        auto limits = formats::TgsDecodeLimits::telegramSticker();
        limits.maxJsonBytes = kMaximumJsonBytes;
        auto result = formats::decodeTgs(bytes, limits);
        if (!result) return {{}, error(OwnErrorCode::DecodeFailed, "/", result.error.message)};
        decoded = std::move(result.json);
        json = decoded;
    } else if (format == formats::AssetFormat::LottieJson) {
        if (bytes.size() > kMaximumJsonBytes)
            return {{}, error(OwnErrorCode::ResourceLimit, "/", "own JSON byte limit exceeded")};
        json = {reinterpret_cast<const char*>(bytes.data()), bytes.size()};
    } else {
        return {{}, error(OwnErrorCode::UnknownFormat, "/", "unknown own asset format")};
    }
    const auto parsed = formats::detail::readOwnJson(json, {65536, 32});
    if (!parsed) {
        return {{}, error(parsed.code == formats::detail::OwnJsonReadCode::ResourceLimit
                              ? OwnErrorCode::ResourceLimit : OwnErrorCode::InvalidJson,
                          parsed.path, "own JSON admission failed")};
    }
    auto prepared = detail::prepareOwnMotionAsset(*parsed.document);
    if (!prepared) {
        return {{}, error(prepared.code == detail::OwnNativeEllipsePrepareCode::InvalidModel
                              ? OwnErrorCode::UnsupportedAsset : OwnErrorCode::PreparationFailed,
                          prepared.path.empty() ? "/" : std::move(prepared.path),
                          prepared.message.empty() ? "own asset preparation failed"
                                                   : std::move(prepared.message))};
    }
    const auto& model = *prepared.prepared->model;
    if (model.logicalWidth < 1 || model.logicalWidth > 4096 ||
        model.logicalHeight < 1 || model.logicalHeight > 4096 ||
        model.totalFrames < 2 || model.totalFrames > 18000 ||
        !std::isfinite(model.frameRate) || model.frameRate <= 0) {
        return {{}, error(OwnErrorCode::UnsupportedAsset, "/", "own asset metadata outside supported bounds")};
    }
    const auto duration = static_cast<float>(model.totalFrames - 1U)
        / static_cast<float>(model.frameRate);
    if (!std::isfinite(duration) || duration <= 0) {
        return {{}, error(OwnErrorCode::UnsupportedAsset, "/fr",
                          "own frame rate produces no finite positive duration")};
    }
    auto impl = std::make_unique<OwnAsset::Impl>();
    impl->metadata = {static_cast<std::size_t>(model.logicalWidth),
                      static_cast<std::size_t>(model.logicalHeight), model.totalFrames,
                      model.frameRate};
    impl->vectorAuthored = bool(prepared.prepared->vectorAuthored);
    impl->prepared = std::move(prepared.prepared);
    return {std::shared_ptr<const OwnAsset>(new OwnAsset(std::move(impl))), {}};
}

std::optional<OwnTargetSize> fitOwnTarget(
    const OwnAsset& asset, std::size_t boxWidth, std::size_t boxHeight) noexcept {
    if (boxWidth == 0 || boxHeight == 0) return std::nullopt;
    const auto& metadata = asset.metadata();
    if (!asset.impl_->vectorAuthored) return OwnTargetSize{boxWidth, boxHeight};
    const auto width = metadata.logicalWidth;
    const auto height = metadata.logicalHeight;
    if (width == 0 || height == 0) return std::nullopt;
    const auto widthQuotient = boxWidth / width;
    const auto heightQuotient = boxHeight / height;
    const auto widthRemainder = boxWidth % width;
    const auto heightRemainder = boxHeight % height;
    // Remainder cross-products are at most 4095 squared. Dividing first also
    // avoids overflowing a large physical box times an authored dimension.
    const bool widthLimits = widthQuotient < heightQuotient ||
        (widthQuotient == heightQuotient &&
         widthRemainder * height <= heightRemainder * width);
    const auto fittedWidth = widthLimits ? boxWidth
        : heightQuotient * width + (heightRemainder * width) / height;
    const auto fittedHeight = widthLimits
        ? widthQuotient * height + (widthRemainder * height) / width : boxHeight;
    if (fittedWidth == 0 || fittedHeight == 0) return std::nullopt;
    return OwnTargetSize{fittedWidth, fittedHeight};
}

struct OwnPlayback::Impl final {
    std::shared_ptr<const OwnAsset> asset;
    std::unique_ptr<detail::OwnNativeEllipsePlayback> playback;
};
OwnPlayback::OwnPlayback(std::unique_ptr<Impl> impl) : impl_(std::move(impl)) {}
OwnPlayback::~OwnPlayback() = default;
const OwnAsset& OwnPlayback::asset() const noexcept { return *impl_->asset; }
void OwnPlayback::play(runtime::MotionTime now) noexcept { impl_->playback->play(now); }
void OwnPlayback::pause(runtime::MotionTime now) noexcept { impl_->playback->pause(now); }
void OwnPlayback::resume(runtime::MotionTime now) noexcept { impl_->playback->resume(now); }
void OwnPlayback::stop() noexcept { impl_->playback->stop(); }
void OwnPlayback::seekNormalized(double position, runtime::MotionTime now) noexcept {
    impl_->playback->seekNormalized(position, now);
}
void OwnPlayback::setControlledProgress(double position) noexcept {
    impl_->playback->setControlledProgress(position);
}
void OwnPlayback::setDirection(runtime::PlaybackDirection direction, runtime::MotionTime now) noexcept {
    impl_->playback->setDirection(direction, now);
}
bool OwnPlayback::setPlaybackRate(double rate, runtime::MotionTime now) noexcept {
    return impl_->playback->setPlaybackRate(rate, now);
}
void OwnPlayback::setLoopMode(runtime::PlaybackLoopMode mode) noexcept {
    impl_->playback->setLoopMode(mode);
}
runtime::PlaybackSnapshot OwnPlayback::playbackSnapshot(runtime::MotionTime now) const noexcept {
    return impl_->playback->playbackSnapshot(now);
}
double OwnPlayback::frameRate() const noexcept { return impl_->playback->frameRate(); }
double OwnPlayback::durationSeconds() const noexcept { return impl_->playback->durationSeconds(); }
OwnPlayback::Evaluation OwnPlayback::evaluateAt(runtime::MotionTime now,
                                                std::size_t width, std::size_t height) {
    auto result = impl_->playback->evaluateAt(now, width, height);
    if (!result) {
        const auto code = result.code == detail::OwnNativeEllipseFrameCode::InvalidViewport
                              ? OwnErrorCode::InvalidViewport
                          : result.code == detail::OwnNativeEllipseFrameCode::UnsupportedNumericOutput
                              || result.code == detail::OwnNativeEllipseFrameCode::UnsupportedClipping
                              ? OwnErrorCode::UnsupportedFrame : OwnErrorCode::EvaluationFailed;
        return {{}, error(code, "/", std::move(result.message))};
    }
    return {std::move(result.scene), {}};
}
OwnPlaybackResult createOwnPlayback(std::shared_ptr<const OwnAsset> asset) {
    if (!asset || !asset->impl_ || !asset->impl_->prepared)
        return {{}, error(OwnErrorCode::InvalidAsset, "/", "invalid own asset")};
    auto created = detail::OwnNativeEllipsePlayback::create(asset->impl_->prepared);
    if (!created) return {{}, error(created.code == detail::OwnNativeEllipseCreateCode::IdentityExhausted
                                     ? OwnErrorCode::IdentityExhausted : OwnErrorCode::PreparationFailed,
                                 "/", std::move(created.message))};
    auto impl = std::make_unique<OwnPlayback::Impl>();
    impl->asset = std::move(asset);
    impl->playback = std::move(created.playback);
    return {std::shared_ptr<OwnPlayback>(new OwnPlayback(std::move(impl))), {}};
}

namespace {
const player::detail::PlayerSourceOps kPublicOwnOps{
    .frameRate = [](const void* object) noexcept { return static_cast<const OwnPlayback*>(object)->frameRate(); },
    .snapshot = [](const void* object, runtime::MotionTime time) noexcept {
        return static_cast<const OwnPlayback*>(object)->playbackSnapshot(time);
    },
    .play = [](void* object, runtime::MotionTime time) noexcept { static_cast<OwnPlayback*>(object)->play(time); },
    .pause = [](void* object, runtime::MotionTime time) noexcept { static_cast<OwnPlayback*>(object)->pause(time); },
    .resume = [](void* object, runtime::MotionTime time) noexcept { static_cast<OwnPlayback*>(object)->resume(time); },
    .stop = [](void* object) noexcept { static_cast<OwnPlayback*>(object)->stop(); },
    .seekNormalized = [](void* object, double position, runtime::MotionTime time) noexcept {
        static_cast<OwnPlayback*>(object)->seekNormalized(position, time);
    },
    .setControlledProgress = [](void* object, double position) noexcept {
        static_cast<OwnPlayback*>(object)->setControlledProgress(position);
    },
    .setDirection = [](void* object, runtime::PlaybackDirection direction, runtime::MotionTime time) noexcept {
        static_cast<OwnPlayback*>(object)->setDirection(direction, time);
    },
    .setPlaybackRate = [](void* object, double rate, runtime::MotionTime time) noexcept {
        return static_cast<OwnPlayback*>(object)->setPlaybackRate(rate, time);
    },
    .setLoopMode = [](void* object, runtime::PlaybackLoopMode mode) noexcept {
        static_cast<OwnPlayback*>(object)->setLoopMode(mode);
    },
};
}
player::PlayerAddResult addOwnPlayback(player::Player& player,
                                      std::shared_ptr<OwnPlayback> playback,
                                      runtime::MotionTime now, player::PlayerEntryOptions options) {
    player::detail::PlayerSource source{
        .owner = std::move(playback), .ops = &kPublicOwnOps,
        .runtimeInstance = nullptr, .runtimeHandle = {},
    };
    return player::detail::PlayerSourceAccess::add(player, std::move(source), now, options);
}
std::shared_ptr<OwnPlayback> ownPlayback(const player::Player& player,
                                        player::PlayerHandle handle) noexcept {
    auto source = player::detail::PlayerSourceAccess::source(player, handle);
    if (source.ops != &kPublicOwnOps || !source.owner) return {};
    auto* typed = static_cast<OwnPlayback*>(source.owner.get());
    return {std::move(source.owner), typed};
}
} // namespace avemotion::render
