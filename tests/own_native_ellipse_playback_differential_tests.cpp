#include "NativeEllipseCertificate.hpp"
#include "OwnNativeEllipsePlayback.hpp"
#include "OwnNativeEllipseModelTestData.hpp"
#include "OwnNativeEllipseStreamTestData.hpp"
#include "support/NativeEllipseOracle.hpp"
#include "support/NativeEllipseTestAssets.hpp"
#include "support/OwnNativeEllipseStreamComparison.hpp"
#include "avemotion/render/RenderPlanner.hpp"
#include "avemotion/runtime/Runtime.hpp"

#include <rlottie.h>

#include <array>
#include <cmath>
#include <fstream>
#include <iostream>
#include <iterator>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>

namespace {
using namespace avemotion;
using Playback = render::detail::OwnNativeEllipsePlayback;
using runtime::MotionTime;
constexpr std::uint64_t kOrdinaryPlanId = 0x26A26003ULL;
void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}
MotionTime at(double seconds) { return MotionTime::fromSeconds(seconds); }
double ordinaryTelegramPosition(double position) {
    return std::isfinite(position) ? position : 0.0;
}

void equalSnapshot(const runtime::PlaybackSnapshot& a,
                   const runtime::PlaybackSnapshot& b, const std::string& label) {
    require(a.status == b.status && a.direction == b.direction
        && a.loopMode == b.loopMode && a.clip == b.clip
        && a.playbackRate == b.playbackRate
        && a.normalizedPosition == b.normalizedPosition
        && a.frameIndex == b.frameIndex && a.revision == b.revision
        && a.completed == b.completed, label + " full control snapshot");
}

void matrixCase(const test::NativeEllipseTestAsset& testAsset,
                std::size_t width, std::size_t height, std::size_t& comparisons) {
    const auto label = testAsset.name + " " + std::to_string(width) + "x" + std::to_string(height);
    auto owner = test::prepareOwnEllipseForTest(testAsset.json);
    auto own = Playback::create(owner);
    require(bool(own), label + " own playback created");
    runtime::Runtime runtime;
    auto loaded = runtime.loadLottieJson(testAsset.json, label);
    require(bool(loaded), label + " ordinary asset loaded");
    auto created = runtime.createInstance(loaded.asset);
    require(bool(created), label + " ordinary instance created");
    auto& old = *created.instance;
    auto ordinaryTelegram = rlottie::Animation::loadFromData(
        testAsset.json, label + "-direct-telegram", {}, false);
    require(bool(ordinaryTelegram), label + " direct cache-disabled Telegram loaded");
    require(own.playback->durationSeconds() == ordinaryTelegram->duration()
        && own.playback->frameRate() == ordinaryTelegram->frameRate(),
        label + " duration and rate match ordinary Telegram");
    const auto decoded = runtime::detail::decodeNativeEllipseInput(testAsset.json);
    require(bool(decoded), label + " input decoded");
    test::NativeEllipseOracle oracle(testAsset.json);
    const auto binding = runtime::detail::bindNativeEllipseModel(*decoded.input, *oracle.model());
    require(bool(binding), label + " ordinary source binding");
    require(oracle.model().get() != owner->model.get(), label + " independent ordinary model");
    const test::OwnNativeEllipseStreamComparison comparator;
    render::MotionRenderPlanner ordinaryPlanner, ownPlanner;
    std::uint64_t ownId = 0;
    std::size_t ordinal = 0;
    auto sample = [&](const char* event, MotionTime now) {
        const auto name = label + " " + event;
        const auto beforeOwn = own.playback->playbackSnapshot(now);
        const auto beforeOld = old.playbackSnapshot(now);
        equalSnapshot(beforeOwn, beforeOld, name + " before");
        const auto frame = ordinaryTelegram->frameAtPos(
            ordinaryTelegramPosition(beforeOld.normalizedPosition));
        require(beforeOwn.frameIndex == frame, name + " ordinary Telegram mapped frame");
        auto emitted = own.playback->evaluateAt(now, width, height);
        auto ordinaryEmission = old.evaluateAt(now, width, height);
        require(bool(emitted) && bool(ordinaryEmission), name + " successful matched emissions");
        auto reference = oracle.freshScene(frame, width, height);
        auto candidate = *emitted.scene;
        ++ordinal;
        if (ordinal == 1) ownId = candidate.instanceId;
        require(reference.evaluationSequence == ordinal
            && candidate.evaluationSequence == ordinal
            && candidate.frameIndex == frame && candidate.instanceId == ownId
            && candidate.sourceAssetHash == test::ownRawHash(testAsset.json),
            name + " original scene metadata and attempt history");
        reference.instanceId = kOrdinaryPlanId;
        const auto sceneDifference = comparator.sceneDifference(reference, *binding.binding,
            candidate, owner->authored->binding, kOrdinaryPlanId);
        require(sceneDifference.empty(), name + " full scene: " + sceneDifference);
        auto ordinaryPlan = ordinaryPlanner.build(
            std::make_shared<const runtime::EvaluatedScene>(reference));
        auto ownPlan = ownPlanner.build(
            std::make_shared<const runtime::EvaluatedScene>(candidate));
        require(bool(ordinaryPlan) && bool(ownPlan), name + " plans built");
        const auto planDifference = comparator.planDifference(ordinaryPlan.plan,
            *binding.binding, ownPlan.plan, owner->authored->binding, kOrdinaryPlanId);
        require(planDifference.empty(), name + " full plan: " + planDifference);
        equalSnapshot(own.playback->playbackSnapshot(now), old.playbackSnapshot(now),
            name + " after successful commit");
        ++comparisons;
    };
    for (const double position : {-1.0, 0.0, 0.249999, 0.25, 0.5, 1.0, 2.0,
             std::numeric_limits<double>::quiet_NaN(),
             std::numeric_limits<double>::infinity()}) {
        require(own.playback->frameAtPosition(position)
            == ordinaryTelegram->frameAtPos(ordinaryTelegramPosition(position)),
            label + " clamped, nonfinite or fractional ordinary Telegram mapped frame");
    }
    sample("stopped", at(0));
    own.playback->play(at(0)); old.play(at(0));
    sample("forward-quarter", at(0.25));
    own.playback->pause(at(0.25)); old.pause(at(0.25));
    sample("paused", at(0.75));
    own.playback->resume(at(1)); old.resume(at(1));
    sample("resumed", at(1.25));
    own.playback->seekNormalized(0.5, at(2)); old.seekNormalized(0.5, at(2));
    sample("seek", at(2));
    own.playback->setControlledProgress(0.375); old.setControlledProgress(0.375);
    sample("controlled", at(3));
    own.playback->resume(at(3)); old.resume(at(3));
    own.playback->setDirection(runtime::PlaybackDirection::Reverse, at(3));
    old.setDirection(runtime::PlaybackDirection::Reverse, at(3));
    require(own.playback->setPlaybackRate(2.0, at(3)) == old.setPlaybackRate(2.0, at(3)),
        label + " rate acceptance");
    sample("reverse-rate", at(3.1));
    own.playback->stop(); old.stop();
    sample("reverse-stop", at(4));
    own.playback->play(at(4)); old.play(at(4));
    sample("reverse-anchor", at(4));
    sample("reverse-loop", at(4.2));
    own.playback->setDirection(runtime::PlaybackDirection::Forward, at(5));
    old.setDirection(runtime::PlaybackDirection::Forward, at(5));
    own.playback->stop(); old.stop();
    own.playback->setLoopMode(runtime::PlaybackLoopMode::Once);
    old.setLoopMode(runtime::PlaybackLoopMode::Once);
    require(own.playback->setPlaybackRate(1.0, at(5)) == old.setPlaybackRate(1.0, at(5)),
        label + " normal rate accepted");
    own.playback->play(at(5)); old.play(at(5));
    sample("once-before", at(5.5));
    sample("once-complete", at(7));
    sample("once-held", at(8));
    own.playback->seekNormalized(0.25, at(8)); old.seekNormalized(0.25, at(8));
    sample("seek-after-holding", at(8));
    require(ordinal == 14, label + " deterministic scene/plan sample count");
}

void failedTerminalRetry(const std::string& json) {
    auto own = Playback::create(test::prepareOwnEllipseForTest(json));
    runtime::Runtime runtime;
    auto loaded = runtime.loadLottieJson(json, "own-playback-failure");
    auto created = runtime.createInstance(loaded.asset);
    require(bool(own) && bool(loaded) && bool(created), "retry pair created");
    auto& old = *created.instance;
    own.playback->setLoopMode(runtime::PlaybackLoopMode::Once);
    old.setLoopMode(runtime::PlaybackLoopMode::Once);
    own.playback->play(at(0)); old.play(at(0));
    equalSnapshot(own.playback->playbackSnapshot(at(2)), old.playbackSnapshot(at(2)),
        "pure pending Holding");
    const auto revision = own.playback->playbackSnapshot(at(2)).revision;
    auto failed = own.playback->evaluateAt(at(2), 0, 512);
    auto oldFailed = old.evaluateAt(at(2), 0, 512);
    require(!failed && !oldFailed, "matched terminal invalid viewport fails");
    equalSnapshot(own.playback->playbackSnapshot(at(2)), old.playbackSnapshot(at(2)),
        "failure leaves revision uncommitted");
    require(own.playback->playbackSnapshot(at(2)).revision == revision,
        "invalid attempt does not commit own completion");
    require(bool(own.playback->evaluateAt(at(2), 512, 512))
        && bool(old.evaluateAt(at(2), 512, 512)), "matched retry succeeds");
    equalSnapshot(own.playback->playbackSnapshot(at(3)), old.playbackSnapshot(at(3)),
        "successful retry commits Holding");
    require(own.playback->playbackSnapshot(at(3)).revision == revision + 1,
        "retry commits only once");
}

void ownCallsDoNotTouchReferenceCounters(const std::string& json) {
    runtime::Runtime runtime;
    const auto before = runtime.diagnostics();
    auto own = Playback::create(test::prepareOwnEllipseForTest(json));
    require(bool(own), "counter playback created");
    own.playback->play(at(0));
    for (int i = 0; i < 4; ++i)
        require(bool(own.playback->evaluateAt(at(i * 0.25), 512, 512)),
            "counter own emission succeeds");
    const auto after = runtime.diagnostics();
    require(before.referenceMetadataSessionsCreated == after.referenceMetadataSessionsCreated
        && before.referenceSceneSessionsCreated == after.referenceSceneSessionsCreated
        && before.referenceModelSessionsCreated == after.referenceModelSessionsCreated
        && before.referenceCpuSessionsCreated == after.referenceCpuSessionsCreated
        && before.referenceSceneSamples == after.referenceSceneSamples
        && before.referenceModelSamples == after.referenceModelSamples
        && before.sceneEvaluations == after.sceneEvaluations
        && before.modelEvaluations == after.modelEvaluations,
        "own-only calls produce no Runtime reference counter activity");
}
} // namespace

int main() {
    try {
        std::ifstream input(std::string(AVEMOTION_FIXTURE_DIR) + "/telegram_sticker_basic.json");
        require(bool(input), "fixture readable");
        const std::string json(std::istreambuf_iterator<char>{input}, {});
        ownCallsDoNotTouchReferenceCounters(json);
        failedTerminalRetry(json);
        const std::array<std::pair<std::size_t, std::size_t>, 4> viewports{{
            {512, 512}, {256, 256}, {384, 256}, {256, 384}}};
        std::size_t comparisons = 0;
        for (const auto& asset : test::nativeEllipseTestAssets(json)) {
            for (const auto [width, height] : viewports)
                matrixCase(asset, width, height, comparisons);
            std::cout << "CASE " << asset.name << " comparisons=56\n";
        }
        require(comparisons == 840, "15 cases x 4 viewports x 14 trace emissions");
        std::cout << "MATRIX comparisons=" << comparisons
            << " shared-control snapshots are wiring-only evidence\n";
    } catch (const std::exception& error) {
        std::cerr << "FAILED: " << error.what() << '\n';
        return 1;
    }
}
