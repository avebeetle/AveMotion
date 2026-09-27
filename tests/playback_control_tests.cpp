#include "PlaybackControl.hpp"

#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace {

using namespace avemotion::runtime;
using namespace avemotion::runtime::detail;

[[noreturn]] void fail(const char* message) {
    std::cerr << "FAILED: " << message << '\n';
    std::exit(EXIT_FAILURE);
}

void require(bool condition, const char* message) {
    if (!condition) fail(message);
}

bool near(double actual, double expected) {
    return std::abs(actual - expected) <= 1.0e-12;
}

MotionTime ms(std::int64_t value) {
    return MotionTime::fromNanoseconds(value * 1'000'000);
}

void timelineAndTransitions() {
    // Catches a lost absolute-time anchor, accumulated pause time, and visual jumps on direction changes.
    PlaybackControl control;
    playPlayback(control, ms(0));
    require(near(snapshotPlayback(control, 1.0, ms(250)).normalizedPosition, 0.25),
        "play at 250 ms must present .25");
    pausePlayback(control, 1.0, ms(250));
    require(near(snapshotPlayback(control, 1.0, ms(750)).normalizedPosition, 0.25),
        "pause must freeze at .25");
    resumePlayback(control, ms(500));
    require(near(snapshotPlayback(control, 1.0, ms(750)).normalizedPosition, 0.5),
        "resume must exclude paused time");
    setPlaybackDirection(control, PlaybackDirection::Reverse, 1.0, ms(750));
    require(near(snapshotPlayback(control, 1.0, ms(750)).normalizedPosition, 0.5),
        "direction change must preserve position");
    require(near(snapshotPlayback(control, 1.0, ms(850)).normalizedPosition, 0.4),
        "reverse at 850 ms must present .4");

    // Catches wrapping the reverse terminal endpoint before the first tick.
    stopPlayback(control);
    playPlayback(control, ms(0));
    require(near(snapshotPlayback(control, 1.0, ms(0)).normalizedPosition, 1.0),
        "reverse play must expose terminal endpoint at anchor");
    require(near(snapshotPlayback(control, 1.0, ms(250)).normalizedPosition, 0.75),
        "reverse play at 250 ms must present .75");

    // Catches controlled progress advancing with wall time or resume jumping away.
    controlPlayback(control, 0.375);
    const auto controlled = snapshotPlayback(control, 1.0, ms(5000));
    require(controlled.status == PlaybackStatus::Controlled
            && near(controlled.normalizedPosition, 0.375),
        "controlled .375 must ignore time");
    resumePlayback(control, ms(500));
    require(near(snapshotPlayback(control, 1.0, ms(500)).normalizedPosition, 0.375),
        "resume must anchor controlled .375");
}

void onceCompletionAndRevisions() {
    // Catches snapshot mutating raw control, early completion commit, and repeat commits.
    PlaybackControl control;
    const auto initial = snapshotPlayback(control, 1.0, ms(0));
    require(initial.status == PlaybackStatus::Stopped
            && initial.direction == PlaybackDirection::Forward
            && initial.loopMode == PlaybackLoopMode::Loop
            && initial.clip.value == 0U
            && near(initial.playbackRate, 1.0)
            && near(initial.normalizedPosition, 0.0)
            && initial.frameIndex == 0U
            && initial.revision == 1U && !initial.completed,
        "default snapshot fields changed");
    setPlaybackLoopMode(control, PlaybackLoopMode::Once);
    require(control.revision == 2U, "loop setter must increment revision");
    playPlayback(control, ms(0));
    require(control.revision == 3U, "play must increment revision");
    playPlayback(control, ms(100));
    require(control.revision == 3U, "repeated play must be inert");
    const auto complete = snapshotPlayback(control, 1.0, ms(1200));
    require(complete.status == PlaybackStatus::Holding && complete.completed
            && near(complete.normalizedPosition, 1.0) && complete.revision == 3U
            && control.status == PlaybackStatus::Playing && control.revision == 3U,
        "once snapshot must report completion without committing");
    commitPlaybackCompletion(control, complete, ms(1200));
    require(control.status == PlaybackStatus::Holding && control.revision == 4U,
        "completion commit must enter Holding once");
    commitPlaybackCompletion(control, complete, ms(1200));
    require(control.revision == 4U, "repeated completion commit must be inert");
    resumePlayback(control, ms(1300));
    require(control.status == PlaybackStatus::Playing
            && near(snapshotPlayback(control, 1.0, ms(1300)).normalizedPosition, 1.0),
        "resume Holding must not restart at zero");
    commitPlaybackCompletion(control, snapshotPlayback(control, 1.0, ms(1300)), ms(1300));
    playPlayback(control, ms(1400));
    require(near(snapshotPlayback(control, 1.0, ms(1400)).normalizedPosition, 0.0),
        "play Holding must restart at direction endpoint");
    commitPlaybackCompletion(control, snapshotPlayback(control, 1.0, ms(2500)), ms(2500));
    seekPlayback(control, 0.6, ms(2500));
    require(control.status == PlaybackStatus::Paused
            && near(snapshotPlayback(control, 1.0, ms(4000)).normalizedPosition, 0.6),
        "seek Holding must become Paused at requested position");
}

void mutationRevisions() {
    // Catches idempotent verbs incrementing and existing setters ceasing to increment.
    PlaybackControl control;
    pausePlayback(control, 1.0, ms(0));
    require(control.revision == 1U, "pause while stopped must be inert");
    setPlaybackDirection(control, PlaybackDirection::Forward, 1.0, ms(0));
    require(control.revision == 1U, "same direction must be inert");
    resumePlayback(control, ms(0));
    require(control.revision == 2U, "resume must increment revision");
    resumePlayback(control, ms(100));
    require(control.revision == 2U, "repeated resume must be inert");
    pausePlayback(control, 1.0, ms(250));
    require(control.revision == 3U, "pause must increment revision");
    pausePlayback(control, 1.0, ms(500));
    require(control.revision == 3U, "repeated pause must be inert");
    seekPlayback(control, 0.25, ms(500));
    seekPlayback(control, 0.25, ms(500));
    require(control.revision == 5U, "repeated seek must increment twice");
    controlPlayback(control, 0.25);
    controlPlayback(control, 0.25);
    require(control.revision == 7U, "repeated controlled setter must increment twice");
    require(setPlaybackRate(control, 1.0, 1.0, ms(500)), "valid unchanged rate rejected");
    require(control.revision == 8U, "unchanged rate setter must increment");
    setPlaybackLoopMode(control, PlaybackLoopMode::Loop);
    require(control.revision == 9U, "unchanged loop setter must increment");
    stopPlayback(control);
    stopPlayback(control);
    require(control.revision == 11U, "repeated stop must increment twice");
}

void invalidAndExtremeInputs() {
    // Catches invalid rates mutating state and nonfinite positions escaping the clamp.
    PlaybackControl control;
    playPlayback(control, ms(0));
    const double invalidRates[] = {-1.0, 0.0,
        std::numeric_limits<double>::quiet_NaN(),
        std::numeric_limits<double>::infinity(),
        -std::numeric_limits<double>::infinity()};
    for (double rate : invalidRates) {
        require(!setPlaybackRate(control, rate, 1.0, ms(250)),
            "invalid rate was accepted");
        require(control.revision == 2U && near(control.playbackRate, 1.0)
                && near(control.anchorPosition, 0.0),
            "invalid rate changed control");
    }
    const double inputs[] = {-1.0, 2.0,
        std::numeric_limits<double>::quiet_NaN(),
        std::numeric_limits<double>::infinity(),
        -std::numeric_limits<double>::infinity()};
    const double expected[] = {0.0, 1.0, 0.0, 0.0, 0.0};
    for (std::size_t i = 0; i < 5U; ++i) {
        seekPlayback(control, inputs[i], ms(0));
        require(near(snapshotPlayback(control, 1.0, ms(0)).normalizedPosition, expected[i]),
            "seek failed finite normalized clamp");
        controlPlayback(control, inputs[i]);
        require(near(snapshotPlayback(control, 1.0, ms(900)).normalizedPosition, expected[i]),
            "controlled progress failed finite normalized clamp");
    }

    // Catches reversed delta operands and integer subtraction overflow.
    PlaybackControl backward;
    playPlayback(backward, ms(0));
    require(near(snapshotPlayback(backward, 1.0, ms(-250)).normalizedPosition, 0.75),
        "backward presentation time must wrap to .75 in Loop");
    setPlaybackLoopMode(backward, PlaybackLoopMode::Once);
    const auto before = snapshotPlayback(backward, 1.0, ms(-250));
    require(near(before.normalizedPosition, 0.0) && !before.completed,
        "backward Once time must clamp to zero without completion");
    PlaybackControl endpoints;
    playPlayback(endpoints, MotionTime::fromNanoseconds(INT64_MIN));
    setPlaybackLoopMode(endpoints, PlaybackLoopMode::Once);
    const auto far = snapshotPlayback(endpoints, 1.0,
        MotionTime::fromNanoseconds(INT64_MAX));
    require(far.completed && near(far.normalizedPosition, 1.0),
        "INT64 endpoint time must complete without subtraction overflow");

    // Catches changed invalid-duration and extreme finite-rate policy.
    PlaybackControl duration;
    playPlayback(duration, ms(0));
    const auto zero = snapshotPlayback(duration, 0.0, ms(250));
    const auto invalid = snapshotPlayback(duration,
        std::numeric_limits<double>::infinity(), ms(250));
    require(zero.completed && invalid.completed
            && near(zero.normalizedPosition, 0.0)
            && near(invalid.normalizedPosition, 0.0),
        "invalid durations must report completion at anchor");
    require(setPlaybackRate(duration, std::numeric_limits<double>::max(), 1.0, ms(0)),
        "extreme finite positive rate must be accepted");
    require(near(snapshotPlayback(duration, 1.0, ms(250)).normalizedPosition, 0.0),
        "extreme rate Loop nonfinite modulo must present zero");
}

} // namespace

int main() {
    timelineAndTransitions();
    onceCompletionAndRevisions();
    mutationRevisions();
    invalidAndExtremeInputs();
    std::cout << "AveMotion private playback control tests passed\n";
    return EXIT_SUCCESS;
}
