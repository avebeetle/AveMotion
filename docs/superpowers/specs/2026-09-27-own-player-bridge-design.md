# Part26J — own playback in the existing Player

## Intent and approval

Architectural follow-on to Part26I, base084d760. User asks to continue toward
static Avelabs integration without unnecessary machinery, using Telegram ideas
where applicable. Controller approves this written design under the existing
explicit delegation; this is not a claim that the user reviewed this file.

Success: the existing Player schedules real own ellipse playbacks, with one
unchanged cadence/visibility/callback implementation and no fabricated Runtime
Instance. This is the scheduler prerequisite to a truthful own visual host pilot,
not a claim that this stage already displays own frames in Avelabs.

## Exploration and alternatives

Player.cpp owns the existing registry, handles, cadence, Freeze/KeepUp and host
callback coalescing, but directly calls Instance for eleven playback operations.
I's own object already implements those operations. MotionWorker currently owns
one Qt timer and uses Player, then Instance::renderCpuFrame -> QImage. Own output
is a scene/plan, so swapping the CPU call alone cannot display own pixels.

Selected: retain the actual Player implementation, replace only its private
source binding with an owning erased source plus static operation table. A
Rendering-private pair of typed registration/lookup functions binds own playback
to the same Player. An opaque private friend access declaration is the only
installed header change; no public registration overload, producer interface,
new Player wrapper class, scheduler library, timer, worker or runtime loader.

Alternatives: template the whole scheduler over traits (two compiled copies of
the loop, unnecessary here); directly depend Player on own Rendering types or
copy scheduling in the host (dependency cycle or duplicate behavior). Reject.
Rendering may depend privately on Player; Player must not depend on Rendering.

## Private source contract

Add src/player/PlayerSource.hpp. A source owns shared_ptr<void> to the concrete
object and a pointer to a static-lifetime table of noexcept operations:
frameRate, playbackSnapshot(now), play/pause/resume(now), stop(),
seekNormalized(value,now), setControlledProgress(value), setDirection(value,now),
setPlaybackRate(value,now)->bool, setLoopMode(value). No std::function closures,
per-entry adapter heap allocation, virtual producer hierarchy or external plugin
API. This is internal type erasure for two real concrete consumers.

The source also carries optional genuine Runtime Instance pointer/handle for
legacy public projection. Runtime binding preserves its real owner/identity;
own binding leaves both Runtime fields null/invalid. Reject null owner/table or
missing operations before mutating registration state. Duplicate detection uses
the retained concrete object address within one Player. No arbitrary identifier
or fake Instance handle is accepted by the own adapter.

PlayerSourceAccess is a friend with private-header-only add/source lookup entry
points. Public addInstance creates the Runtime table/binding and delegates the
common registration; public instance(handle) returns only a genuine Runtime
owner, null for own/invalid handles. Every existing public method, error code,
Runtime error message, diagnostics, generation policy and tick span lifetime
remains unchanged. Do not opportunistically fix extreme-time/counter quirks.

PlayerEntry retains the source instead of a hardwired Instance shared pointer;
all scheduling/control source calls route through its table. Arithmetic, loop
order, callback reentrancy protection, pending request/coalescing, storage reserve,
automatic pause/resume, late-deadline skipping, completion notification and
destructor/move cancellation remain the same. One implementation is compiled.

## Typed own registration and identities

Add src/render/OwnNativeEllipsePlayer.hpp/.cpp with:

- addOwnNativeEllipsePlayback(Player&, shared_ptr<OwnNativeEllipsePlayback>,
  MotionTime, PlayerEntryOptions={}) -> PlayerAddResult;
- ownNativeEllipsePlayback(const Player&, PlayerHandle) -> shared_ptr<...>.

Lookup validates the exact own operation-table identity before casting; Runtime
entries and stale handles yield null. No second registry/map or per-tick adapter
allocation. The same public Player control verbs/tick/callbacks now drive own
objects through this internal registration. Own ScheduledFrame has a valid
PlayerHandle but null Instance and invalid InstanceHandle; consumer uses typed
lookup, never dereferences the legacy fields. Tick does not evaluate or commit
Holding. Host separately calls own evaluateAt at the chosen presentation time;
only successful emission commits completion, as in I.

Three identity domains stay distinct: PlayerHandle for scheduling, genuine
Runtime handles for legacy entries, own scene.instanceId for own planner/cache.
Mixed registration in one Player is supported, NOT mixed own/Runtime scenes in
one planner/backend cache. Host retains own scene IDs and calls planner forget
or reset and backend clear/domain invalidation on retirement. Player removal
releases its owner and scheduling state, not external graphics caches.
Own linked-allocator lifetime and per-object single-writer rules remain binding.
Callbacks must not re-enter this Player. No new cross-thread safety guarantee.

## Telegram and host source checkpoint

Revisited pinned desktop-app/lib_lottie commit
7d00b5048aff8dd93c0a9721abf50006d20682be on2026-09-27:
https://github.com/desktop-app/lib_lottie/blob/7d00b5048aff8dd93c0a9721abf50006d20682be/lottie/details/lottie_frame_renderer.cpp#L140-L172
Its queueGenerateFrames coalesces work; weak-owner notifications reach the main
thread. Applicable principle: one bounded request path and lifetime-aware host
delivery, already present in Player/MotionWorker. Do not import Telegram queues,
four-frame machinery, code, resources or new dependency/license decisions.
Qt thread-affinity contract: https://doc.qt.io/qt-6.10/threads-qobject.html .
This stage adds no Qt object; existing one-thread/one-timer host stays unchanged.

Next host design must choose actual output: small offscreen Direct2D/WARP
readback into existing QImage is a truthful experimental own-rendered preview,
not GPU-composited QWidget or an optimization result. Native window presentation
is larger DPI/device-loss work and is not silently added here. Both need a
narrow internal build-tree preparation/render boundary before host code changes.

## Scope and proof

Main/root-only Git, no force/rebase/history rewrite, preserve raw/SDD. No host
writes/build/GUI or UI/out recreation in J; no vendor/licenses/dependencies,
goldens/fixtures, public own loader, Player threading, fallback or D2D ownership
changes. Existing public scheduling behavior and Samsung policy stay intact.

TDD then independent task reviews. Task1 functional source-registration/control
tests in no-ref plus existing Runtime Player suites, callback traces and literal
deadline/reason/diagnostic expectations. Task2 own-only no-ref tests and Telegram
mixed trace: Freeze vs KeepUp, hidden user pause, wakeup cancellation/coalescing,
early and late ticks, rates/revisions, Once failed/successful evaluation, stale
handle reuse, source lifetime, callback replacement/destruction/move, cache
retirement. Shared control/scheduler comparisons are wiring proof; literal
expectations and unchanged I/H oracle/pixel suites provide independent checks.
No hidden Runtime ownership/activity in own-only no-reference build/link.

One whole-stage review, at most one combined final fix and one scoped re-review.
Fresh frozen full MSVC none/Telegram/explicit Win32-preview; Samsung targeted
old/own Player and playback, not full Samsung green. Both vendor/TGS integrity.
Actual new private-source include/link/install closure and unchanged host hash;
no installed-consumer/performance/zero-allocation/TSan/hardware GPU claim.

## Self-review

Smallest concrete seam is two typed free functions over existing Player, not a
second facade registry or copied scheduler. The private friend/header addition
and Rendering->Player link are explicit. No own UI picture, unsupported features
or cache/thread guarantees are promised before the next host stage.
