#include "PlayerSource.hpp"
#include "PlaybackControl.hpp"

#include <cstdlib>
#include <iostream>
#include <limits>
#include <memory>
#include <optional>
#include <utility>

namespace {

using namespace avemotion;
using runtime::MotionTime;

struct Source final {
    runtime::detail::PlaybackControl playback;
};

const player::detail::PlayerSourceOps sourceOps{
    .frameRate = [](const void*) noexcept { return 1.0; },
    .snapshot = [](const void* object, MotionTime now) noexcept {
        return runtime::detail::snapshotPlayback(
            static_cast<const Source*>(object)->playback, 1.0, now);
    },
    .play = [](void* object, MotionTime now) noexcept {
        runtime::detail::playPlayback(static_cast<Source*>(object)->playback, now);
    },
    .pause = [](void* object, MotionTime now) noexcept {
        runtime::detail::pausePlayback(static_cast<Source*>(object)->playback, 1.0, now);
    },
    .resume = [](void* object, MotionTime now) noexcept {
        runtime::detail::resumePlayback(static_cast<Source*>(object)->playback, now);
    },
    .stop = [](void* object) noexcept {
        runtime::detail::stopPlayback(static_cast<Source*>(object)->playback);
    },
    .seekNormalized = [](void* object, double value, MotionTime now) noexcept {
        runtime::detail::seekPlayback(static_cast<Source*>(object)->playback, value, now);
    },
    .setControlledProgress = [](void* object, double value) noexcept {
        runtime::detail::controlPlayback(static_cast<Source*>(object)->playback, value);
    },
    .setDirection = [](void* object, runtime::PlaybackDirection value,
                       MotionTime now) noexcept {
        runtime::detail::setPlaybackDirection(
            static_cast<Source*>(object)->playback, value, 1.0, now);
    },
    .setPlaybackRate = [](void* object, double value, MotionTime now) noexcept {
        return runtime::detail::setPlaybackRate(
            static_cast<Source*>(object)->playback, value, 1.0, now);
    },
    .setLoopMode = [](void* object, runtime::PlaybackLoopMode value) noexcept {
        runtime::detail::setPlaybackLoopMode(static_cast<Source*>(object)->playback, value);
    },
};

[[noreturn]] void fail(const char* message) {
    std::cerr << "FAILED: " << message << '\n';
    std::exit(EXIT_FAILURE);
}

void require(bool value, const char* message) {
    if (!value) fail(message);
}

player::detail::PlayerSource makeSource(const std::shared_ptr<Source>& object) {
    return {.owner = object, .ops = &sourceOps};
}

struct HostRecorder final {
    std::uint64_t requests = 0U;
    std::uint64_t schedules = 0U;
    std::uint64_t cancels = 0U;
    std::optional<MotionTime> deadline;

    static void request(void* context) noexcept {
        ++static_cast<HostRecorder*>(context)->requests;
    }
    static void schedule(void* context, MotionTime time) noexcept {
        auto& self = *static_cast<HostRecorder*>(context);
        ++self.schedules;
        self.deadline = time;
    }
    static void cancel(void* context) noexcept {
        auto& self = *static_cast<HostRecorder*>(context);
        ++self.cancels;
        self.deadline.reset();
    }
    player::PlayerHostCallbacks callbacks() noexcept {
        return {this, &request, &schedule, &cancel};
    }
};

void testCadenceAndControls() {
    const auto zero = MotionTime::fromNanoseconds(0);
    const auto late = MotionTime::fromNanoseconds(3'500'000'000LL);
    auto object = std::make_shared<Source>();
    HostRecorder host;
    player::Player scheduler{host.callbacks()};
    const auto added = player::detail::PlayerSourceAccess::add(
        scheduler, makeSource(object), zero);
    require(static_cast<bool>(added), "valid private source registration failed");
    require(host.requests == 1U, "registration did not request one first frame");
    require(player::detail::PlayerSourceAccess::source(scheduler, added.handle)
            .owner.get() == object.get(),
        "private source lookup lost its owner");
    require(!scheduler.instance(added.handle),
        "generic source resolved as a Runtime instance");
    auto tick = scheduler.tick(zero);
    require(tick.frames.size() == 1U,
        "valid source did not return its initial scheduled frame");
    require(tick.frames[0].handle == added.handle,
        "initial frame belongs to a different source");
    require(tick.frames[0].instance == nullptr
            && !tick.frames[0].instanceHandle.valid(),
        "generic source acquired a Runtime identity");
    require(player::hasReason(tick.frames[0].reasons, player::FrameReason::FirstFrame)
            && player::hasReason(tick.frames[0].reasons,
                player::FrameReason::PlaybackChanged),
        "first stopped frame lacks its first/revision reasons");
    require(!tick.nextDeadline, "stopped source scheduled a deadline");
    require(scheduler.tick(zero).frames.empty(),
        "unchanged stopped source returned another frame");

    require(scheduler.play(added.handle, zero), "play failed");
    tick = scheduler.tick(zero);
    require(tick.frames.size() == 1U
            && tick.nextDeadline == MotionTime::fromNanoseconds(1'000'000'000LL),
        "rate-one source did not schedule one-second deadline");
    require(tick.frames[0].playback.status == runtime::PlaybackStatus::Playing,
        "play control did not reach concrete source");
    tick = scheduler.tick(late);
    require(tick.frames.size() == 1U
            && player::hasReason(tick.frames[0].reasons,
                player::FrameReason::TimelineAdvanced)
            && tick.skippedDeadlines == 2U
            && tick.nextDeadline == MotionTime::fromNanoseconds(4'000'000'000LL),
        "late tick did not coalesce to one frame and deadline four");
    require(tick.frames[0].playback.normalizedPosition == 0.5,
        "late tick sampled wrong source position");
    require(scheduler.diagnostics().skippedDeadlines == 2U,
        "skipped deadline diagnostic is wrong");

    const auto requests = host.requests;
    require(scheduler.invalidate(added.handle), "first invalidation failed");
    require(scheduler.invalidate(added.handle), "second invalidation failed");
    require(host.requests == requests + 1U,
        "explicit invalidations did not coalesce host requests");
    tick = scheduler.tick(late);
    require(tick.frames.size() == 1U
            && player::hasReason(tick.frames[0].reasons,
                player::FrameReason::ExplicitInvalidation),
        "coalesced explicit invalidation was lost");

    require(scheduler.pause(added.handle, late), "pause failed");
    require(object->playback.status == runtime::PlaybackStatus::Paused,
        "pause did not reach source");
    require(scheduler.resume(added.handle, late), "resume failed");
    require(object->playback.status == runtime::PlaybackStatus::Playing,
        "resume did not reach source");
    require(scheduler.seekNormalized(added.handle, 0.25, late), "seek failed");
    require(object->playback.anchorPosition == 0.25, "seek did not reach source");
    require(scheduler.setDirection(added.handle,
        runtime::PlaybackDirection::Reverse, late), "direction failed");
    require(object->playback.direction == runtime::PlaybackDirection::Reverse,
        "direction did not reach source");
    require(scheduler.setPlaybackRate(added.handle, 2.0, late), "rate failed");
    require(object->playback.playbackRate == 2.0, "rate did not reach source");
    require(!scheduler.setPlaybackRate(added.handle, 0.0, late),
        "invalid rate was accepted");
    require(scheduler.setLoopMode(added.handle,
        runtime::PlaybackLoopMode::Once, late), "loop mode failed");
    require(object->playback.loopMode == runtime::PlaybackLoopMode::Once,
        "loop mode did not reach source");
    require(scheduler.setControlledProgress(added.handle, 0.75, late),
        "controlled progress failed");
    require(object->playback.status == runtime::PlaybackStatus::Controlled
            && object->playback.anchorPosition == 0.75,
        "controlled progress did not reach source");
    require(scheduler.stop(added.handle, late), "stop failed");
    require(object->playback.status == runtime::PlaybackStatus::Stopped,
        "stop did not reach source");

    const auto storage = scheduler.diagnostics().storageGeneration;
    scheduler.resetDiagnostics();
    const auto diagnostics = scheduler.diagnostics();
    require(diagnostics.ticks == 0U && diagnostics.registrations == 0U
            && diagnostics.storageGeneration == storage
            && diagnostics.registeredInstances == 1U
            && diagnostics.visibleInstances == 1U
            && diagnostics.playingInstances == 0U,
        "diagnostic reset did not preserve current counts/storage");
}

void testRejectionAndStaleHandles() {
    const auto zero = MotionTime::fromNanoseconds(0);
    HostRecorder host;
    player::Player scheduler{host.callbacks()};
    auto object = std::make_shared<Source>();
    auto invalid = makeSource(object);
    invalid.owner.reset();
    require(player::detail::PlayerSourceAccess::add(scheduler, invalid, zero)
            .error.code == player::PlayerErrorCode::InvalidArgument,
        "null owner was accepted");
    invalid = makeSource(object);
    invalid.ops = nullptr;
    require(player::detail::PlayerSourceAccess::add(scheduler, invalid, zero)
            .error.code == player::PlayerErrorCode::InvalidArgument,
        "null table was accepted");
    auto incomplete = sourceOps;
    incomplete.pause = nullptr;
    invalid = makeSource(object);
    invalid.ops = &incomplete;
    require(player::detail::PlayerSourceAccess::add(scheduler, invalid, zero)
            .error.code == player::PlayerErrorCode::InvalidArgument,
        "missing operation was accepted");
    require(player::detail::PlayerSourceAccess::add(scheduler,
        makeSource(object), zero, {.maximumPresentationRate =
            std::numeric_limits<double>::infinity()})
            .error.code == player::PlayerErrorCode::InvalidArgument,
        "invalid maximum presentation rate was accepted");
    require(scheduler.diagnostics().registrations == 0U
            && scheduler.diagnostics().storageGeneration == 0U
            && host.requests == 0U && host.schedules == 0U,
        "rejected source mutated registry or callbacks");

    const auto added = player::detail::PlayerSourceAccess::add(
        scheduler, makeSource(object), zero);
    require(static_cast<bool>(added), "valid source registration failed");
    const auto before = scheduler.diagnostics().registrations;
    const auto callbacksBefore = host.requests;
    require(player::detail::PlayerSourceAccess::add(
        scheduler, makeSource(object), zero).error.code
            == player::PlayerErrorCode::DuplicateInstance,
        "duplicate concrete owner was accepted");
    require(scheduler.diagnostics().registrations == before
            && host.requests == callbacksBefore,
        "duplicate owner changed registry or callbacks");

    const player::PlayerHandle invalidHandle;
    require(!scheduler.play(invalidHandle, zero)
            && !scheduler.invalidate(invalidHandle)
            && !scheduler.removeInstance(invalidHandle, zero)
            && !player::detail::PlayerSourceAccess::source(scheduler, invalidHandle).owner,
        "invalid handle acted on a source");
    require(scheduler.removeInstance(added.handle, zero), "remove failed");
    require(!player::detail::PlayerSourceAccess::source(scheduler, added.handle).owner
            && !scheduler.play(added.handle, zero),
        "stale handle acted on removed source");
    const auto replacement = player::detail::PlayerSourceAccess::add(
        scheduler, makeSource(object), zero);
    require(static_cast<bool>(replacement)
            && replacement.handle.index == added.handle.index
            && replacement.handle.generation != added.handle.generation,
        "slot reuse did not advance generation");
    require(!scheduler.pause(added.handle, zero),
        "stale handle controlled replacement source");
}

void testVisibilityLifetimeAndCallbacks() {
    const auto zero = MotionTime::fromNanoseconds(0);
    const auto later = MotionTime::fromNanoseconds(5'000'000'000LL);
    HostRecorder oldHost;
    HostRecorder newHost;
    std::weak_ptr<Source> weak;
    {
        player::Player scheduler{oldHost.callbacks()};
        auto object = std::make_shared<Source>();
        weak = object;
        const auto added = player::detail::PlayerSourceAccess::add(
            scheduler, makeSource(object), zero, {
                .hiddenTimePolicy = player::HiddenTimePolicy::Freeze});
        require(static_cast<bool>(added), "Freeze source registration failed");
        object.reset();
        require(scheduler.play(added.handle, zero), "Freeze play failed");
        require(oldHost.deadline == MotionTime::fromNanoseconds(1'000'000'000LL),
            "active source did not publish wakeup");
        require(scheduler.setVisible(added.handle, false, zero), "hide failed");
        require(weak.lock()->playback.status == runtime::PlaybackStatus::Paused
                && scheduler.diagnostics().automaticPauses == 1U,
            "Freeze did not pause playing source");
        require(!scheduler.nextDeadline() && oldHost.cancels == 1U,
            "hiding final playing source did not cancel wakeup");
        require(scheduler.pause(added.handle, zero), "manual pause failed");
        require(scheduler.setVisible(added.handle, true, later), "show failed");
        require(weak.lock()->playback.status == runtime::PlaybackStatus::Paused
                && scheduler.diagnostics().automaticResumes == 0U,
            "manual pause was resumed automatically");
        static_cast<void>(scheduler.tick(later));
        const auto requestsBeforeRemoval = oldHost.requests;
        require(scheduler.removeInstance(added.handle, later), "remove failed");
        require(oldHost.requests == requestsBeforeRemoval + 1U,
            "remove did not request a clear-presentation repaint");
        require(weak.expired(), "remove retained source owner");
        require(scheduler.diagnostics().registeredInstances == 0U,
            "remove left current registration count");
        scheduler.setHostCallbacks(newHost.callbacks(), later);
        require(newHost.requests == 1U,
            "callback replacement lost pending clear-presentation repaint");
    }

    HostRecorder removalHost;
    HostRecorder replacementHost;
    std::weak_ptr<Source> removedWeak;
    {
        player::Player scheduler{removalHost.callbacks()};
        auto object = std::make_shared<Source>();
        removedWeak = object;
        const auto added = player::detail::PlayerSourceAccess::add(
            scheduler, makeSource(object), zero);
        require(static_cast<bool>(added), "active removal setup failed");
        require(scheduler.play(added.handle, zero), "active removal play failed");
        static_cast<void>(scheduler.tick(zero));
        require(removalHost.deadline == MotionTime::fromNanoseconds(1'000'000'000LL),
            "active removal lacked final wakeup");
        const auto requestsBeforeRemoval = removalHost.requests;
        object.reset();
        require(scheduler.removeInstance(added.handle, zero),
            "active removal failed");
        require(removedWeak.expired() && !scheduler.nextDeadline()
                && removalHost.cancels == 1U
                && removalHost.requests == requestsBeforeRemoval + 1U,
            "active removal retained owner, wakeup or lost repaint");
        scheduler.setHostCallbacks(replacementHost.callbacks(), zero);
        require(replacementHost.requests == 1U,
            "callback replacement lost active removal repaint");
    }

    HostRecorder keepHost;
    {
        player::Player keep{keepHost.callbacks()};
        auto object = std::make_shared<Source>();
        const auto added = player::detail::PlayerSourceAccess::add(
            keep, makeSource(object), zero, {
                .hiddenTimePolicy = player::HiddenTimePolicy::KeepUp});
        require(static_cast<bool>(added), "KeepUp registration failed");
        require(keep.play(added.handle, zero), "KeepUp play failed");
        require(keep.setVisible(added.handle, false, zero), "KeepUp hide failed");
        require(object->playback.status == runtime::PlaybackStatus::Playing
                && keep.diagnostics().automaticPauses == 0U,
            "KeepUp automatically paused source");
        require(keep.setVisible(added.handle, true, later), "KeepUp show failed");
        require(keep.nextDeadline().has_value(),
            "KeepUp show did not restore deadline");
        HostRecorder replacement;
        keep.setHostCallbacks(replacement.callbacks(), later);
        require(keepHost.cancels >= 1U && replacement.deadline == keep.nextDeadline(),
            "callback replacement did not retire/republish wakeup");
        player::Player moved{std::move(keep)};
        require(moved.nextDeadline() == replacement.deadline,
            "move lost active wakeup");
        require(moved.removeInstance(added.handle, later), "move owner remove failed");
        require(replacement.cancels == 1U,
            "move owner did not cancel final wakeup");
        moved.clear(later);
    }

    HostRecorder deathHost;
    {
        player::Player dying{deathHost.callbacks()};
        auto object = std::make_shared<Source>();
        const auto added = player::detail::PlayerSourceAccess::add(
            dying, makeSource(object), zero);
        require(static_cast<bool>(added), "destructor source registration failed");
        require(dying.play(added.handle, zero), "destructor play failed");
        require(deathHost.deadline.has_value(),
            "destructor case lacks active deadline");
    }
    require(deathHost.cancels == 1U,
        "destructor did not retire final wakeup");

    HostRecorder clearHost;
    player::Player cleared{clearHost.callbacks()};
    auto object = std::make_shared<Source>();
    auto clearWeak = std::weak_ptr<Source>{object};
    const auto added = player::detail::PlayerSourceAccess::add(
        cleared, makeSource(object), zero);
    require(static_cast<bool>(added), "clear source registration failed");
    require(cleared.play(added.handle, zero), "clear source play failed");
    static_cast<void>(cleared.tick(zero));
    const auto clearRequests = clearHost.requests;
    object.reset();
    cleared.clear(zero);
    require(clearWeak.expired() && cleared.diagnostics().registeredInstances == 0U
            && clearHost.cancels == 1U && clearHost.requests == clearRequests + 1U,
        "clear retained source or failed to cancel/repaint");

    HostRecorder assignmentHost;
    HostRecorder incomingHost;
    player::Player assigned{assignmentHost.callbacks()};
    player::Player incoming{incomingHost.callbacks()};
    auto assignedObject = std::make_shared<Source>();
    auto incomingObject = std::make_shared<Source>();
    const auto assignedHandle = player::detail::PlayerSourceAccess::add(
        assigned, makeSource(assignedObject), zero);
    const auto incomingHandle = player::detail::PlayerSourceAccess::add(
        incoming, makeSource(incomingObject), zero);
    require(static_cast<bool>(assignedHandle) && static_cast<bool>(incomingHandle),
        "move-assignment setup failed");
    require(assigned.play(assignedHandle.handle, zero)
            && incoming.play(incomingHandle.handle, zero),
        "move-assignment play failed");
    assigned = std::move(incoming);
    require(assignmentHost.cancels == 1U
            && assigned.nextDeadline() == incomingHost.deadline,
        "move assignment did not retire old callback and retain new schedule");
}

} // namespace

int main() {
    testCadenceAndControls();
    testRejectionAndStaleHandles();
    testVisibilityLifetimeAndCallbacks();
    std::cout << "AveMotion private Player source tests passed\n";
    return EXIT_SUCCESS;
}
