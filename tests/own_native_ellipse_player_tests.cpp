#include "OwnNativeEllipsePlayer.hpp"
#include "OwnNativeEllipseStreamTestData.hpp"
#include "avemotion/render/RenderPlanner.hpp"

#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>

namespace {
using namespace avemotion;
using runtime::MotionTime;
using Playback = render::detail::OwnNativeEllipsePlayback;
using player::FrameReason;

void require(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}
MotionTime at(double seconds) { return MotionTime::fromSeconds(seconds); }
bool near(double a, double b) { return std::abs(a - b) < 1e-6; }
std::shared_ptr<Playback> makePlayback(const std::string& json) {
    auto prepared = test::prepareOwnEllipseForTest(json);
    auto result = Playback::create(std::move(prepared));
    require(bool(result), "own prepared fixture creates playback");
    return {std::move(result.playback)};
}
std::string fixture() {
    std::ifstream input(std::filesystem::path{AVEMOTION_FIXTURE_DIR}
        / "telegram_sticker_basic.json", std::ios::binary);
    require(bool(input), "fixture readable");
    return {std::istreambuf_iterator<char>{input}, {}};
}
const player::ScheduledFrame* frameFor(
    std::span<const player::ScheduledFrame> frames, player::PlayerHandle handle) {
    for (const auto& frame : frames) if (frame.handle == handle) return &frame;
    return nullptr;
}
void ownFrame(const player::ScheduledFrame& frame, player::PlayerHandle handle) {
    require(frame.handle == handle && frame.instance == nullptr
        && !frame.instanceHandle.valid(), "own frame has only Player identity");
}

struct Host final {
    std::size_t requests = 0, schedules = 0, cancels = 0;
    std::optional<MotionTime> deadline;
    static void request(void* data) noexcept { ++static_cast<Host*>(data)->requests; }
    static void schedule(void* data, MotionTime when) noexcept {
        auto& host = *static_cast<Host*>(data); ++host.schedules; host.deadline = when;
    }
    static void cancel(void* data) noexcept {
        auto& host = *static_cast<Host*>(data); ++host.cancels; host.deadline.reset();
    }
    player::PlayerHostCallbacks callbacks() noexcept {
        return {this, request, schedule, cancel};
    }
};

void registrationAndTimeline(const std::string& json) {
    Host host;
    player::Player player{host.callbacks()};
    require(!render::detail::addOwnNativeEllipsePlayback(player, {}, at(0))
        && !render::detail::ownNativeEllipsePlayback(player, {}),
        "null own source and invalid lookup rejected");
    auto source = makePlayback(json);
    const auto added = render::detail::addOwnNativeEllipsePlayback(player, source, at(0));
    require(bool(added), "prepared JSON own playback scheduled");
    const auto handle = added.handle;
    require(render::detail::ownNativeEllipsePlayback(player, handle) == source
        && !player.instance(handle), "typed own owner and no runtime owner");
    require(!render::detail::addOwnNativeEllipsePlayback(player, source, at(0))
        && player.diagnostics().registrations == 1,
        "duplicate own address rejected without registration");
    source.reset();
    auto tick = player.tick(at(0));
    require(tick.frames.size() == 1 && frameFor(tick.frames, handle)
        && player::hasReason(tick.frames[0].reasons, FrameReason::FirstFrame)
        && tick.frames[0].playback.frameIndex == 0,
        "first scheduled own frame zero");
    ownFrame(tick.frames[0], handle);
    require(player.play(handle, at(0)), "own play accepted");
    tick = player.tick(at(0));
    require(frameFor(tick.frames, handle) != nullptr && tick.nextDeadline,
        "play yields frame and deadline");
    const auto deadline = *tick.nextDeadline;
    tick = player.tick(MotionTime::fromNanoseconds(deadline.nanoseconds - 1));
    require(tick.frames.empty(), "early tick emits no own frame");
    tick = player.tick(deadline);
    require(tick.frames.size() == 1
        && player::hasReason(tick.frames[0].reasons, FrameReason::TimelineAdvanced),
        "deadline emits one own frame");
    tick = player.tick(at(0.25));
    require(tick.frames.size() == 1 && tick.frames[0].playback.frameIndex == 15
        && tick.skippedDeadlines > 0, "late tick skips deadlines and selects frame 15");
    ownFrame(tick.frames[0], handle);
    require(player.pause(handle, at(0.25)), "pause accepted");
    tick = player.tick(at(0.25));
    require(tick.frames.size() == 1 && tick.frames[0].playback.frameIndex == 15
        && player::hasReason(tick.frames[0].reasons, FrameReason::PlaybackChanged),
        "pause schedules frozen frame 15");
    tick = player.tick(at(0.75));
    require(tick.frames.empty() && !tick.nextDeadline,
        "paused own source has no deadline");
    require(player.resume(handle, at(0.75)), "resume accepted");
    tick = player.tick(at(1));
    require(tick.frames.size() == 1 && tick.frames[0].playback.frameIndex == 30,
        "resumed own source advances to frame 30");
    require(player.seekNormalized(handle, 0.5, at(1)), "seek accepted");
    tick = player.tick(at(1));
    require(tick.frames.size() == 1 && tick.frames[0].playback.frameIndex == 30,
        "seek schedules anchored frame 30");
    require(player.setDirection(handle, runtime::PlaybackDirection::Reverse, at(1))
        && player.setPlaybackRate(handle, 2.0, at(1))
        && !player.setPlaybackRate(handle, 0.0, at(1))
        && player.setLoopMode(handle, runtime::PlaybackLoopMode::Loop, at(1)),
        "reverse, valid rate, invalid rate and loop use Player controls");
    tick = player.tick(at(1.25));
    require(tick.frames.size() == 1 && tick.frames[0].playback.frameIndex == 0,
        "reverse double-rate reaches first frame");
    require(player.diagnostics().skippedDeadlines > 0
        && player.diagnostics().framesReturned > 0 && host.requests > 0
        && host.schedules > 0, "own activity reaches diagnostics and callbacks");
    auto retained = render::detail::ownNativeEllipsePlayback(player, handle);
    require(player.removeInstance(handle, at(2)) && !player.nextDeadline()
        && !render::detail::ownNativeEllipsePlayback(player, handle),
        "remove cancels own scheduling and invalidates lookup");
    const auto replacement = render::detail::addOwnNativeEllipsePlayback(
        player, retained, at(2));
    require(replacement && replacement.handle != handle
        && !render::detail::ownNativeEllipsePlayback(player, handle)
        && render::detail::ownNativeEllipsePlayback(player, replacement.handle) == retained,
        "generation rejects stale handle after re-registration");
    player.clear(at(2));
    require(!render::detail::ownNativeEllipsePlayback(player, replacement.handle)
        && !player.nextDeadline(), "clear releases registered own source");
}

void visibilityAndRevision(const std::string& json) {
    player::Player player;
    auto frozen = makePlayback(json);
    auto keep = makePlayback(json);
    auto paused = makePlayback(json);
    const auto f = render::detail::addOwnNativeEllipsePlayback(player, frozen, at(0),
        {.hiddenTimePolicy = player::HiddenTimePolicy::Freeze});
    const auto k = render::detail::addOwnNativeEllipsePlayback(player, keep, at(0));
    const auto p = render::detail::addOwnNativeEllipsePlayback(player, paused, at(0),
        {.hiddenTimePolicy = player::HiddenTimePolicy::Freeze});
    require(f && k && p && f.handle != k.handle && k.handle != p.handle,
        "independent own sources have distinct Player handles");
    require(player.play(f.handle, at(0)) && player.play(k.handle, at(0))
        && player.play(p.handle, at(0)) && player.pause(p.handle, at(0.25)),
        "independent own controls accepted");
    (void)player.tick(at(0));
    require(player.setVisible(f.handle, false, at(0.25))
        && player.setVisible(k.handle, false, at(0.25))
        && player.setVisible(p.handle, false, at(0.25)),
        "all own sources can hide");
    (void)player.tick(at(0.25));
    const auto hidden = player.tick(at(0.75));
    require(hidden.frames.empty() && !hidden.nextDeadline
        && near(frozen->playbackSnapshot(at(0.75)).normalizedPosition, 0.25)
        && near(keep->playbackSnapshot(at(0.75)).normalizedPosition, 0.75),
        "hidden KeepUp advances logical time without frames; Freeze stays at quarter");
    require(player.setVisible(f.handle, true, at(1.25))
        && player.setVisible(k.handle, true, at(1.25))
        && player.setVisible(p.handle, true, at(1.25)),
        "all own sources can show");
    auto tick = player.tick(at(1.25));
    require(tick.frames.size() == 3, "shown entries request one frame each");
    for (const auto& frame : tick.frames) ownFrame(frame, frame.handle);
    require(near(frozen->playbackSnapshot(at(1.25)).normalizedPosition, 0.25)
        && near(keep->playbackSnapshot(at(1.25)).normalizedPosition, 0.25)
        && paused->playbackSnapshot(at(1.25)).status == runtime::PlaybackStatus::Paused
        && near(paused->playbackSnapshot(at(1.25)).normalizedPosition, 0.25),
        "Freeze preserves quarter, KeepUp loops logically, user pause remains paused");
    tick = player.tick(at(1.5));
    require(frameFor(tick.frames, f.handle)
        && frameFor(tick.frames, f.handle)->playback.frameIndex == 30,
        "Freeze resumes from quarter to half");
    require(player.diagnostics().automaticPauses == 1
        && player.diagnostics().automaticResumes == 1,
        "only playing Freeze entry auto pauses and resumes");
    frozen->setControlledProgress(0.375);
    tick = player.tick(at(1.5));
    require(frameFor(tick.frames, f.handle)
        && player::hasReason(frameFor(tick.frames, f.handle)->reasons,
            FrameReason::PlaybackChanged), "direct own mutation detected by revision");
    require(player.pause(k.handle, at(1.5))
        && paused->playbackSnapshot(at(2)).status == runtime::PlaybackStatus::Paused,
        "source controls do not bleed between owners");
}

void completionAndResources(const std::string& json) {
    Host host;
    auto owner = makePlayback(json);
    auto weak = std::weak_ptr<Playback>{owner};
    render::MotionRenderPlanner planner;
    std::shared_ptr<const runtime::EvaluatedScene> scene;
    render::MotionRenderPlan plan;
    {
        player::Player player{host.callbacks()};
        const auto added = render::detail::addOwnNativeEllipsePlayback(player, owner, at(0));
        require(added && player.setLoopMode(added.handle, runtime::PlaybackLoopMode::Once, at(0))
            && player.play(added.handle, at(0)), "Once own registered");
        owner.reset();
        (void)player.tick(at(0));
        auto tick = player.tick(at(2));
        require(tick.frames.size() == 1 && tick.frames[0].playback.completed
            && player::hasReason(tick.frames[0].reasons, FrameReason::Completion)
            && !tick.nextDeadline, "Once at two seconds emits completion with no deadline");
        const auto before = tick.frames[0].playback.revision;
        auto live = render::detail::ownNativeEllipsePlayback(player, added.handle);
        require(live && live->playbackSnapshot(at(2)).revision == before,
            "schedule snapshot does not commit own completion");
        const auto failed = live->evaluateAt(at(2), 0, 512);
        require(!failed && failed.code == render::detail::OwnNativeEllipseFrameCode::InvalidViewport
            && live->playbackSnapshot(at(2)).revision == before,
            "invalid viewport fails without commit");
        require(player.tick(at(2)).frames.empty(), "failed evaluation has no implicit Player retry");
        require(player.invalidate(added.handle), "host explicitly invalidates after failure");
        tick = player.tick(at(2));
        require(tick.frames.size() == 1
            && player::hasReason(tick.frames[0].reasons, FrameReason::ExplicitInvalidation),
            "explicit invalidation requests retry frame");
        auto valid = live->evaluateAt(at(2), 512, 512);
        require(bool(valid) && valid.scene->frameIndex == 60,
            "valid host evaluation emits terminal own scene");
        scene = std::make_shared<const runtime::EvaluatedScene>(*valid.scene);
        auto built = planner.build(scene);
        require(bool(built), "own scene builds own-domain plan");
        plan = built.plan;
        require(live->playbackSnapshot(at(2)).revision == before + 1,
            "successful terminal evaluation commits exactly one revision");
        require(bool(live->evaluateAt(at(2), 512, 512))
            && live->playbackSnapshot(at(2)).revision == before + 1,
            "repeat evaluation does not recommit");
        live.reset();
        require(player.removeInstance(added.handle, at(2)) && weak.expired()
            && !player.nextDeadline(), "remove releases final registered owner");
    }
    require(scene && scene->assetModel && plan.sourceScene && plan.sourceScene->assetModel,
        "retained own scene and plan survive Player owner destruction");
    auto other = makePlayback(json);
    other->setControlledProgress(0.25);
    auto otherFirstScene = other->evaluateAt(at(2), 512, 512);
    require(bool(otherFirstScene) && otherFirstScene.scene->instanceId != scene->instanceId
        && !otherFirstScene.scene->instanceHandle.valid()
        && !scene->instanceHandle.valid(),
        "second real own playback has distinct own scene identity");
    auto otherFirst = planner.build(*otherFirstScene.scene);
    require(bool(otherFirst) && otherFirst.plan.firstPlan
        && otherFirst.plan.stamp.planSequence == 1,
        "second own identity starts independent planner sequence");
    auto otherSecondScene = other->evaluateAt(at(2), 512, 512);
    require(bool(otherSecondScene)
        && otherSecondScene.scene->evaluationSequence == 2,
        "second own playback advances its real scene sequence");
    auto otherSecond = planner.build(*otherSecondScene.scene);
    require(bool(otherSecond) && !otherSecond.plan.firstPlan
        && otherSecond.plan.stamp.planSequence == 2,
        "second own planner state continues before retirement");
    const auto id = scene->instanceId;
    planner.forgetInstance(id);
    auto rebuilt = planner.build(scene);
    require(bool(rebuilt) && rebuilt.plan.firstPlan
        && rebuilt.plan.stamp.planSequence == 1,
        "planner forget restarts retired own resource state");
    auto otherThirdScene = other->evaluateAt(at(2), 512, 512);
    require(bool(otherThirdScene)
        && otherThirdScene.scene->evaluationSequence == 3,
        "untouched own playback produces next real scene");
    auto otherThird = planner.build(*otherThirdScene.scene);
    require(bool(otherThird) && !otherThird.plan.firstPlan
        && otherThird.plan.stamp.planSequence == 3
        && otherThird.plan.stamp.instanceId == otherFirst.plan.stamp.instanceId
        && otherThird.plan.fingerprints.plan == otherSecond.plan.fingerprints.plan
        && !otherThird.plan.visualChanged
        && otherThird.plan.statistics.geometryUpdateCount == 0,
        "forgetting one own ID preserves other ID sequence and resources");
    require(host.requests > 0 && host.schedules > 0 && host.cancels > 0,
        "registered owner routes host wakeup and cancellation callbacks");
}

void callbackAndOwnerRetirement(const std::string& json) {
    Host original, replacement;
    std::weak_ptr<Playback> weak;
    {
        player::Player player{original.callbacks()};
        auto owner = makePlayback(json);
        weak = owner;
        const auto added = render::detail::addOwnNativeEllipsePlayback(player, owner, at(0));
        require(added && player.play(added.handle, at(0)),
            "owner scheduled for callback lifecycle");
        owner.reset();
        (void)player.tick(at(0));
        require(original.deadline && !weak.expired(),
            "Player retains own owner and publishes wakeup");
        player.setHostCallbacks(replacement.callbacks(), at(0));
        require(original.cancels > 0 && replacement.deadline,
            "callback replacement cancels previous wakeup and schedules successor");
        player.clear(at(0));
        require(weak.expired() && !replacement.deadline && replacement.cancels > 0,
            "clear releases owner and cancels replacement wakeup");
        auto second = makePlayback(json);
        weak = second;
        const auto secondAdded = render::detail::addOwnNativeEllipsePlayback(
            player, second, at(0));
        require(secondAdded && player.play(secondAdded.handle, at(0)),
            "new own source schedules after clear");
        second.reset();
        (void)player.tick(at(0));
        require(!weak.expired() && replacement.deadline,
            "new source retained until Player destruction");
    }
    require(weak.expired() && !replacement.deadline && replacement.cancels > 1,
        "Player destruction releases owner and cancels wakeup");
}
} // namespace

int main() {
    try {
        const auto json = fixture();
        registrationAndTimeline(json);
        visibilityAndRevision(json);
        completionAndResources(json);
        callbackAndOwnerRetirement(json);
        std::cout << "own native ellipse player: all checks passed\n";
    } catch (const std::exception& error) {
        std::cerr << "FAILED: " << error.what() << '\n';
        return 1;
    }
}
