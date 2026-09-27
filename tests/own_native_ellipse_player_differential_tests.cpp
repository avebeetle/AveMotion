#include "OwnNativeEllipsePlayer.hpp"
#include "OwnNativeEllipseStreamTestData.hpp"
#include "avemotion/runtime/Runtime.hpp"

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

void require(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}
MotionTime at(double seconds) { return MotionTime::fromSeconds(seconds); }
std::string fixture() {
    std::ifstream input(std::filesystem::path{AVEMOTION_FIXTURE_DIR}
        / "telegram_sticker_basic.json", std::ios::binary);
    require(bool(input), "fixture readable");
    return {std::istreambuf_iterator<char>{input}, {}};
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
void compareSnapshot(const runtime::PlaybackSnapshot& old,
                     const runtime::PlaybackSnapshot& own) {
    require(old.status == own.status && old.direction == own.direction
        && old.loopMode == own.loopMode && old.clip == own.clip
        && old.playbackRate == own.playbackRate
        && old.normalizedPosition == own.normalizedPosition
        && old.frameIndex == own.frameIndex && old.revision == own.revision
        && old.completed == own.completed, "real Runtime and own snapshots align");
}
void compareTick(player::Player& player,
                 player::PlayerHandle oldHandle,
                 player::PlayerHandle ownHandle,
                 runtime::Instance* oldInstance,
                 MotionTime now,
                 std::size_t expectedCount,
                 std::optional<std::size_t> expectedFrame = {}) {
    const auto tick = player.tick(now);
    require(tick.frames.size() == expectedCount, "same Player emits expected mixed frame count");
    if (expectedCount == 0) return;
    require(expectedCount == 2, "mixed comparison expects paired frames");
    const player::ScheduledFrame *old = nullptr, *own = nullptr;
    for (const auto& frame : tick.frames) {
        if (frame.handle == oldHandle) old = &frame;
        if (frame.handle == ownHandle) own = &frame;
    }
    require(old && own && old->instance == oldInstance
        && old->instanceHandle == oldInstance->handle()
        && own->instance == nullptr && !own->instanceHandle.valid(),
        "legacy and own identity projections remain distinct");
    require(old->reasons == own->reasons && old->visible == own->visible,
        "shared scheduler produces matched reasons and visibility");
    compareSnapshot(old->playback, own->playback);
    if (expectedFrame) require(old->playback.frameIndex == *expectedFrame,
        "mixed tick uses independent frame literal");
}
void mixedTrace(const std::string& json) {
    runtime::Runtime runtime;
    const auto loaded = runtime.loadLottieJson(json, "own-player-mixed");
    require(bool(loaded), "real Runtime fixture loads");
    auto created = runtime.createInstance(loaded.asset);
    require(bool(created), "real Runtime instance creates");
    std::shared_ptr<runtime::Instance> legacy{std::move(created.instance)};
    auto prepared = test::prepareOwnEllipseForTest(json);
    auto made = render::detail::OwnNativeEllipsePlayback::create(prepared);
    require(bool(made), "real own fixture creates");
    std::shared_ptr<render::detail::OwnNativeEllipsePlayback> own{
        std::move(made.playback)};
    Host host;
    player::Player player{host.callbacks()};
    const auto oldAdded = player.addInstance(legacy, at(0), {
        .maximumPresentationRate = 30.0,
        .hiddenTimePolicy = player::HiddenTimePolicy::Freeze});
    const auto ownAdded = render::detail::addOwnNativeEllipsePlayback(player, own, at(0), {
        .maximumPresentationRate = 30.0,
        .hiddenTimePolicy = player::HiddenTimePolicy::Freeze});
    require(oldAdded && ownAdded && oldAdded.handle != ownAdded.handle
        && player.instance(oldAdded.handle) == legacy
        && !player.instance(ownAdded.handle)
        && !render::detail::ownNativeEllipsePlayback(player, oldAdded.handle)
        && render::detail::ownNativeEllipsePlayback(player, ownAdded.handle) == own,
        "typed lookups preserve mixed owner domains");
    compareTick(player, oldAdded.handle, ownAdded.handle, legacy.get(), at(0), 2, 0);
    require(player.play(oldAdded.handle, at(0)) && player.play(ownAdded.handle, at(0)),
        "both real sources play through one Player");
    compareTick(player, oldAdded.handle, ownAdded.handle, legacy.get(), at(0), 2, 0);
    require(player.nextDeadline() && host.deadline == player.nextDeadline(),
        "mixed pair shares next host wakeup");
    compareTick(player, oldAdded.handle, ownAdded.handle, legacy.get(), at(0.01), 0);
    compareSnapshot(legacy->playbackSnapshot(at(0.01)), own->playbackSnapshot(at(0.01)));
    compareTick(player, oldAdded.handle, ownAdded.handle, legacy.get(), at(0.25), 2, 15);
    require(player.diagnostics().skippedDeadlines > 0,
        "late mixed tick accounts skipped deadlines");
    require(player.pause(oldAdded.handle, at(0.25))
        && player.pause(ownAdded.handle, at(0.25)), "both sources pause");
    compareTick(player, oldAdded.handle, ownAdded.handle, legacy.get(), at(0.25), 2, 15);
    compareTick(player, oldAdded.handle, ownAdded.handle, legacy.get(), at(0.75), 0);
    compareSnapshot(legacy->playbackSnapshot(at(0.75)), own->playbackSnapshot(at(0.75)));
    require(player.resume(oldAdded.handle, at(0.75))
        && player.resume(ownAdded.handle, at(0.75)), "both sources resume");
    compareTick(player, oldAdded.handle, ownAdded.handle, legacy.get(), at(1), 2, 30);
    require(player.seekNormalized(oldAdded.handle, 0.5, at(1))
        && player.seekNormalized(ownAdded.handle, 0.5, at(1))
        && player.setDirection(oldAdded.handle, runtime::PlaybackDirection::Reverse, at(1))
        && player.setDirection(ownAdded.handle, runtime::PlaybackDirection::Reverse, at(1))
        && player.setPlaybackRate(oldAdded.handle, 2.0, at(1))
        && player.setPlaybackRate(ownAdded.handle, 2.0, at(1))
        && player.setLoopMode(oldAdded.handle, runtime::PlaybackLoopMode::Loop, at(1))
        && player.setLoopMode(ownAdded.handle, runtime::PlaybackLoopMode::Loop, at(1)),
        "both sources accept identical controls");
    compareTick(player, oldAdded.handle, ownAdded.handle, legacy.get(), at(1.25), 2, 0);
    require(player.setVisible(oldAdded.handle, false, at(1.25))
        && player.setVisible(ownAdded.handle, false, at(1.25)), "mixed entries hide");
    compareTick(player, oldAdded.handle, ownAdded.handle, legacy.get(), at(1.25), 2);
    require(player.setVisible(oldAdded.handle, true, at(2.25))
        && player.setVisible(ownAdded.handle, true, at(2.25)), "mixed entries show");
    compareTick(player, oldAdded.handle, ownAdded.handle, legacy.get(), at(2.25), 2, 0);
    require(player.diagnostics().registrations == 2
        && player.diagnostics().automaticPauses == 2
        && player.diagnostics().automaticResumes == 2
        && host.requests > 0 && host.schedules > 0 && host.cancels > 0,
        "mixed callbacks and diagnostics share scheduler path");
    require(player.removeInstance(ownAdded.handle, at(2.25))
        && !render::detail::ownNativeEllipsePlayback(player, ownAdded.handle)
        && player.instance(oldAdded.handle) == legacy,
        "retiring own entry leaves Runtime entry intact");
}
} // namespace

int main() {
    try {
        mixedTrace(fixture());
        std::cout << "own native ellipse player differential: all checks passed\n";
    } catch (const std::exception& error) {
        std::cerr << "FAILED: " << error.what() << '\n';
        return 1;
    }
}
