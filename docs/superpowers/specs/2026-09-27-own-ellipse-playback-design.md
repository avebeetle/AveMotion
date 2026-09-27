# Part26I — own ellipse playback, without a second scheduler

## Intent, classification and approval

Architectural follow-on to completed Part26H (base345f1b6): accept an explicit
host presentation time, reuse AveMotion's existing playback control, and emit the
already admitted own ellipse scene. This is the smallest useful bridge toward
static Avelabs integration. The user's latest constraint is to avoid unnecessary
work and build an optimized professional implementation, with Telegram behavior
as a reference. Do not manufacture performance claims from architectural choices.

Controller approval under the user's explicit delegation of reversible design,
plan and execution decisions; this is not a claim of direct user review of this
document. No ordinary confirmation is pending. Date2026-09-27.

## Exploration and alternatives

Runtime.cpp currently embeds PlaybackControl, absolute-time resolution and
mutation logic. These are first-party code, but only the reference-backed
Instance can reach their full behavior. Player.cpp is coupled to Instance and
owns the existing cadence/visibility/callback policy. Recreating it would be a
second scheduler, not a necessary part of making the own stream time-driven.

1. **Selected:** extract only the existing first-party playback control into a
   private shared component; delegate existing Runtime calls without changing
   behavior; a thin Rendering-private own playback object owns an H stream and
   uses that component. Keep Player unchanged. Cost: a narrowly reviewed edit
   to Runtime.cpp and one small wrapper, with regression gates.
2. Copy the playback algorithm into the own wrapper: smaller initial edit but
   duplicate state transitions and future fixes. Reject.
3. Generalize Player to arbitrary producers now, or invent MotionService/timer
   orchestration: larger public/lifetime surface before a host seam is agreed.
   Defer to the next separate design, using the concrete host inventory.

At each subsequent architecture boundary, inspect the relevant pinned Telegram
source before adopting an idea; record source and applicability, not a wholesale
copy. This is a workflow checkpoint, not a new periodic automation. For timing:
Local Telegram oracle contract is inspectable in
third_party/rlottie/telegram/source/src/lottie/lottiemodel.h: duration is
(totalFrames-1)/floatFrameRate (float division, then widened to double), and normalized frame mapping truncates
position*(totalFrames-1). Our admitted root starts at0, totalFrames is2..10000,
and the own model already retains the required float-derived frame rate.

## Global constraints

- Direct main and scoped ordinary commits/pushes; root owns ALL Git writes. No force-push, rebase, amend or history rewrite. One product writer; preserve raw/SDD evidence.
- No vendor, dependency, license/notice, fixture or golden changes; no dependency installation or Windows/settings/security/automation actions.
- No Avelabs writes/builds/GUI or UI/out recreation. Preserve its accepted Release and current unrelated work.
- No public loader/API, Runtime handles, Player code/threading, Direct2D ownership, ANGLE/backend coverage or public fallback changes. New interfaces are private/not installed; reference-linked install prohibition stays.
- Own reader/admission/model/stream and old certificate/stream semantics remain unchanged. No larger primitive subset, new corpus, generic producer framework, new scheduler, timer, worker or cache service.
- No full Lottie/SVG, complete host playback, hardware GPU, zero-allocation, TSan or speedup claim. Timing control is functional behavior, not a performance measurement.

## Shared playback control

Create src/runtime/PlaybackControl.hpp/.cpp in runtime::detail. Move the current
control state and first-party math/transitions there, retaining operation order,
normalization, defaults, revision behavior and completion semantics. Use a plain
private control plus production-used helper functions; no public export and no
reference/vendor include. Do not add generic callbacks, virtual interfaces or
test-only setters. Duration is an explicit argument where the current Instance
used asset metadata. Snapshot helper leaves frame mapping to the caller.

Preserve existing Runtime reference build behavior and no-reference guards:
play/pause/resume remain no-ops without reference; no-ref snapshot still reports
default position/frame/completion; no-ref setters retain their previous branch
behavior. Keep large-frame fallback and Samsung rounding in Instance unchanged.
No-reference availability of the private helper must NOT become a public loader
or Instance capability claim.

Core policy is existing absolute-time behavior: anchor time/position, finite
normalized clamp (nonfinite->0), positive finite rates only, forward/reverse,
loop/once, play vs resume, stopped/paused/controlled/holding. Reverse at its
terminal anchor exposes1 before wrapping. Snapshot is read-only even when it
reports Holding. Only successful evaluateAt commits Once completion and revision.
Time subtraction uses the existing long-double operands before subtraction to
avoid signed integer overflow. Do not silently change existing extreme-value
or out-of-order-time behavior as part of extraction.

## Own playback wrapper

Rendering-private OwnNativeEllipsePlayback.hpp/.cpp accepts only a sealed
shared_ptr<const OwnNativeEllipsePreparedAsset> via a typed factory. Validate
preparation through H stream construction; errors retain the underlying stream
creation code/message (including exhausted identity), rather than a fake ready
object. Reject invalid input. Noncopyable/nonmovable, single-writer, no host
pointer/callback/timer/thread or Runtime Instance/handle manufactured.

Expose the same control verbs as Instance: play(now), pause(now), resume(now),
stop(), seekNormalized(position,now), setControlledProgress(position),
setDirection(direction,now), setPlaybackRate(rate,now), setLoopMode(mode), plus
playbackSnapshot(now), frameAtPosition(position), durationSeconds(), frameRate()
and evaluateAt(now,width,height). Use existing MotionTime, PlaybackSnapshot and
enums, not a parallel public type family. Own mapping is Telegram truncation in
all build variants, never inherited from a Samsung macro. Own duration uses the
float frame rate and (N-1), not N: exactly
double(float(N-1) / floatFrameRate), preserving the upstream arithmetic type.
No direct integer-frame API needed on wrapper:
H already owns that operation.

evaluateAt resolves one snapshot, emits that integer frame through its H stream,
and commits completion only if emission succeeds. Return the existing H frame
result/error/owned scene; do not copy scene arrays or rebuild preparation. The
wrapper owns the stream/preparation; returned scenes/plans remain valid after
wrapper and external source destruction. New objects have distinct H identities.
Failed viewport evaluation neither commits Holding nor damages previous history.

H's own-only planner/backend domains, linked allocator lifetime, single-writer
objects, caller forget/reset and retained-result rules remain explicit at the
wrapper API. This wrapper does not solve producer namespaces or evict caches.
Visibility/freeze/keep-up/presentation deadlines stay with the future scheduler
seam; do not add a second implementation here.

## Proof

Task1: literal private-control tests in reference=none must first fail against
compiling stubs, then cover default/revisions, control transitions, repeated
commands, pause/resume anchoring, reverse terminal, loop/once, seek after Holding,
controlled progress, invalid rates/nonfinite normalization, out-of-order and
INT64 endpoint times. Existing Runtime playback/frame mapping/Player suites
prove delegation. Include direct before/after legacy characterization where
existing tests lack the required transition. Do not derive expected positions
with the implementation under test. No-ref public Runtime tests stay intact.

Task2: literal own JSON/TGS->prepare->playback->scene tests in reference=none,
functional RED before implementation. Pin N=61/fr=60 duration1s and literal
frame indices at0,0.25,0.5,1, reverse, pause/resume and invalid viewport at Once
completion. Assert revision before/after failed/successful emission and repeated
holding. Check separate instances, retained scenes/plans after owner destruction,
shared own-only planner first/history isolation, zero reference dependency.

Telegram differential tests use all15 existing ellipse variants. Compare own
duration/frame mapping to directly loaded ordinary Telegram and playback
snapshot fields against existing Instance over a deterministic command/time
trace. Since control code is shared, this is wiring regression, NOT independent
proof of timeline math: literal tests provide that proof. Independently sampled
ordinary scenes at the expected mapped frames prove scene/plan output using H's
unchanged approved normalization/comparators. Explicit nonfinite/clamped and
fractional-frame-rate mapping, reverse endpoints, loop/once, seek, failure and
access order. Existing H full parity/pixel tests remain unchanged and run in the
final gates; no new rasterizer implementation or repeated pixel matrix needed.

## Gates and handoff

TDD, one writer, independent task reviews, one whole-stage review. Retain raw
unique RED/GREEN commands/results, root frozen-HEAD configure/incremental-build/
full CTest none, Telegram, windows-msvc-win32-preview; Samsung targeted playback/
frame-mapping/Player execution because extraction affects it (known unrelated
Polystar failures are not new green claims). Both vendor integrity remains intact.
Check actual new-source include/link/install closure, no-reference build and
unmodified host checkpoint. No clean rebuild/performance claim.

Next separately design minimal scheduler/host seam using the existing Player and
Avelabs event loop, then isolated static host acceptance. Do not skip to editing
the user's accepted UI build. Completion report names implemented behavior and
remaining limitations, commits, commands, counts, and decision costs.

## Self-review

No duplicate scheduler or numeric control algorithm, fake Instance, broader loader
or host change. Exact timing/variant policy and success-only transition are fixed;
independent literals prevent a shared-code differential from becoming tautological.
Controller approves this bounded design and proceeds to the detailed plan.
