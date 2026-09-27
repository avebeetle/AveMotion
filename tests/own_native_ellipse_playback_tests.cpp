#include "OwnNativeEllipsePlayback.hpp"
#include "OwnNativeEllipseStreamTestData.hpp"
#include "OwnNativeEllipseModelTestData.hpp"
#include "avemotion/formats/Tgs.hpp"
#include "avemotion/render/RenderPlanner.hpp"
#include "avemotion/runtime/Runtime.hpp"

#include <array>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <type_traits>

namespace {
using namespace avemotion;
using Playback = render::detail::OwnNativeEllipsePlayback;
using runtime::MotionTime;
void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
MotionTime at(double seconds) { return MotionTime::fromSeconds(seconds); }

void literalsAndLifecycle(const std::string& json) {
    static_assert(!std::is_copy_constructible_v<Playback>);
    static_assert(!std::is_move_constructible_v<Playback>);
    const auto invalid = Playback::create(nullptr);
    require(!invalid && invalid.code == render::detail::OwnNativeEllipseCreateCode::InvalidPreparedAsset
        && invalid.message == "incomplete own ellipse preparation" && !invalid.playback,
        "null owner preserves H failure");
    auto owner = test::prepareOwnEllipseForTest(json);
    auto made = Playback::create(owner);
    require(bool(made), "sealed owner creates own playback");
    auto& playback = *made.playback;
    // Wrong N or wrong frame-rate arithmetic must fail these independent literals.
    require(playback.durationSeconds() == 1.0 && playback.frameRate() == 60.0,
        "61 frames at 60 fps last one second");
    for (const auto [position, expected] : std::array<std::pair<double, std::size_t>, 4>{
             {{0.0, 0}, {0.25, 15}, {0.5, 30}, {1.0, 60}}}) {
        require(playback.frameAtPosition(position) == expected, "Telegram truncation literal");
        playback.setControlledProgress(position);
        const auto snapshot = playback.playbackSnapshot(at(17));
        require(snapshot.status == runtime::PlaybackStatus::Controlled
            && snapshot.normalizedPosition == position && snapshot.frameIndex == expected,
            "controlled position resolves literal frame");
        const auto emitted = playback.evaluateAt(at(17), 512, 512);
        require(bool(emitted) && emitted.scene->frameIndex == expected
            && emitted.scene->drawItems.size() == 1,
            "controlled time emits corresponding real geometry");
        if (expected == 0 || expected == 30) {
            const auto& bounds = emitted.scene->drawItems[0].path.controlBounds;
            require(bounds.left == (expected == 0 ? 120.0F : 196.0F)
                && bounds.top == 196.0F
                && bounds.right == (expected == 0 ? 240.0F : 316.0F)
                && bounds.bottom == 316.0F,
                "time-mapped frame emits hand-checked ellipse bounds");
        }
    }
    require(playback.frameAtPosition(-2.0) == 0
        && playback.frameAtPosition(2.0) == 60
        && playback.frameAtPosition(std::numeric_limits<double>::quiet_NaN()) == 0
        && playback.frameAtPosition(std::numeric_limits<double>::infinity()) == 0
        && playback.frameAtPosition(0.25 - 1e-6) == 14
        && playback.frameAtPosition(0.25 + 1e-6) == 15,
        "position clamp, nonfinite and truncation threshold");

    playback.stop();
    playback.setLoopMode(runtime::PlaybackLoopMode::Once);
    playback.play(at(0));
    require(playback.playbackSnapshot(at(0.25)).frameIndex == 15,
        "play advances absolute presentation time");
    playback.pause(at(0.25));
    require(playback.playbackSnapshot(at(9)).frameIndex == 15,
        "pause freezes at anchored frame");
    playback.resume(at(10));
    require(playback.playbackSnapshot(at(10.25)).frameIndex == 30,
        "resume uses new anchor time");
    playback.seekNormalized(0.5, at(20));
    require(playback.playbackSnapshot(at(20)).frameIndex == 30,
        "seek updates present frame");
    require(playback.setPlaybackRate(2.0, at(20)), "valid rate accepted");
    require(!playback.setPlaybackRate(0.0, at(20)), "invalid rate rejected");
    playback.setDirection(runtime::PlaybackDirection::Reverse, at(20));
    require(playback.playbackSnapshot(at(20.25)).frameIndex == 0,
        "reverse rate reaches first frame");
    playback.stop();
    require(playback.playbackSnapshot(at(30)).frameIndex == 60,
        "reverse stop anchors terminal frame");
    playback.setLoopMode(runtime::PlaybackLoopMode::Loop);
    playback.play(at(30));
    require(playback.playbackSnapshot(at(30)).frameIndex == 60,
        "reverse loop exposes endpoint at anchor");
    playback.setDirection(runtime::PlaybackDirection::Forward, at(30));
    playback.stop();
    require(playback.setPlaybackRate(1.0, at(30)), "normal rate restored");
    playback.setLoopMode(runtime::PlaybackLoopMode::Once);
    playback.play(at(40));
    const auto pending = playback.playbackSnapshot(at(41));
    require(pending.status == runtime::PlaybackStatus::Holding && pending.completed
        && pending.frameIndex == 60,
        "pure snapshot reports uncommitted Once completion");
    const auto failed = playback.evaluateAt(at(41), 0, 512);
    require(!failed && failed.code == render::detail::OwnNativeEllipseFrameCode::InvalidViewport,
        "invalid viewport rejects terminal emission");
    require(playback.playbackSnapshot(at(41)).revision == pending.revision,
        "failed emission does not commit Holding");
    const auto completed = playback.evaluateAt(at(41), 512, 512);
    require(bool(completed) && completed.scene->frameIndex == 60
        && completed.scene->evaluationSequence == 6,
        "successful retry emits terminal frame");
    const auto held = playback.playbackSnapshot(at(42));
    require(held.status == runtime::PlaybackStatus::Holding && held.completed
        && held.revision == pending.revision + 1,
        "successful completion commits exactly one revision");
    require(bool(playback.evaluateAt(at(43), 512, 512))
        && playback.playbackSnapshot(at(43)).revision == held.revision,
        "repeated Holding evaluation does not recommit");
}

void sharedOwnerPlannerAndTgs(const std::string& json) {
    auto owner = test::prepareOwnEllipseForTest(json);
    auto first = Playback::create(owner);
    auto second = Playback::create(owner);
    require(bool(first) && bool(second), "two controls share sealed preparation");
    first.playback->setControlledProgress(0);
    second.playback->setControlledProgress(0.5);
    auto a0 = first.playback->evaluateAt(at(0), 512, 512);
    auto b0 = second.playback->evaluateAt(at(0), 512, 512);
    require(bool(a0) && bool(b0) && a0.scene->instanceId != b0.scene->instanceId
        && a0.scene->frameIndex == 0 && b0.scene->frameIndex == 30
        && a0.scene->evaluationSequence == 1 && b0.scene->evaluationSequence == 1
        && a0.scene->sourceAssetHash == test::ownRawHash(json)
        && b0.scene->sourceAssetHash == test::ownRawHash(json)
        && !a0.scene->assetHandle.valid() && !a0.scene->instanceHandle.valid()
        && !b0.scene->assetHandle.valid() && !b0.scene->instanceHandle.valid(),
        "own identities, controls and sequences are independent");
    render::MotionRenderPlanner planner;
    auto pa = planner.build(*a0.scene);
    auto pb = planner.build(*b0.scene);
    require(bool(pa) && bool(pb) && pa.plan.firstPlan && pb.plan.firstPlan,
        "shared planner partitions first plans by own identity");
    first.playback->setControlledProgress(0.25);
    auto a1 = first.playback->evaluateAt(at(1), 512, 512);
    auto pa1 = planner.build(*a1.scene);
    require(bool(a1) && bool(pa1) && pa1.plan.stamp.planSequence == 2
        && a1.scene->evaluationSequence == 2 && !a1.scene->changes.firstEvaluation,
        "successful history advances only first playback");
    auto b1 = second.playback->evaluateAt(at(1), 512, 512);
    require(bool(b1) && b1.scene->evaluationSequence == 2
        && !b1.scene->changes.firstEvaluation,
        "second playback retains separate history");
    auto saved = pa.plan;
    auto savedScene = *a0.scene;
    const auto aid = savedScene.instanceId;
    planner.forgetInstance(aid);
    auto rebuilt = planner.build(*a0.scene);
    require(bool(rebuilt) && rebuilt.plan.firstPlan && rebuilt.plan.stamp.planSequence == 1,
        "planner forget rebuilds own identity");
    first.playback.reset(); second.playback.reset(); owner.reset();
    require(saved.sourceScene && saved.sourceScene->assetModel
        && saved.sourceScene->drawItems.size() == 1
        && savedScene.drawItems.size() == 1 && savedScene.assetModel,
        "retained scenes and plans survive playback and external owner destruction");
    const auto tgs = formats::decodeTgsFile(
        std::filesystem::path{AVEMOTION_TGS_DIR} / "telegram_sticker_basic.tgs");
    require(bool(tgs), "existing TGS decoded");
    auto sticker = Playback::create(test::prepareOwnEllipseForTest(tgs.json));
    require(bool(sticker), "TGS creates playback");
    sticker.playback->setControlledProgress(0.5);
    auto scene = sticker.playback->evaluateAt(at(0), 512, 512);
    require(bool(scene) && scene.scene->frameIndex == 30 && scene.scene->drawItems.size() == 1,
        "TGS decoded bytes travel through own time driven emission");
}
} // namespace

int main() {
    try {
        std::ifstream input(std::string(AVEMOTION_FIXTURE_DIR) + "/telegram_sticker_basic.json");
        require(bool(input), "fixture readable");
        const std::string json(std::istreambuf_iterator<char>{input}, {});
        literalsAndLifecycle(json);
        sharedOwnerPlannerAndTgs(json);
        std::cout << "own native ellipse playback: all checks passed\n";
    } catch (const std::exception& error) {
        std::cerr << "FAILED: " << error.what() << '\n';
        return 1;
    }
}
