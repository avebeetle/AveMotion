# Own UI resource costs and bounded reuse

## Authority and outcome

The user accepts the previous public-own integration as implemented, with native
visual acceptance of the new icon still open. Ordinary reversible choices,
written design/plan approval, main-in-place scoped commits and ordinary push are
delegated. This is a new bounded stage, not a repeat of the accepted integration.

First close its two public-contract findings. Then measure independent UI
elements with the existing measurement approach. Select **one** useful resource
improvement from those results, prove its correctness, and compare the same
workload before/after. A slow or memory-heavy baseline is a valid result.

## Constraints

- E is C:/Users/USER/Desktop/AveMotion; H is D:/rvc/c++/DragonianVoice/Avelabs-UI.
- Windows x64, Qt 6.10.0, C++20, static Release /MT; reference variant none for own host.
- No GPU stage, new scheduler, large cache, universal resource manager or format extension.
- Existing Player, evaluator, backend, async preparation and independent playback remain.
- Published images remain immutable while retained by any consumer.
- No docking, physics, transparency, visual-quality or public fallback-policy changes.
- No dependency installation, Windows/settings changes, history rewriting or new licensing decisions.
- Preserve frozen five-file motion-baseline kit, prior binaries, raw evidence and original artwork hashes.
- Native new-icon/mixed-monitor DPR acceptance remains open; automated paint is not monitor presentation.
- One product implementer at a time; TDD and independent task review before scoped commit/push.

## 1. Public API hardening

The terminal projection contract remains unchanged. Holding/completed snapshots
and Player Completion requests describe the requested terminal state, not a
successful scene emission. Failed evaluation leaves revision uncommitted and
does not automatically requeue a frame. A host can explicitly invalidate the
Player entry to request a retry. A successful terminal evaluation commits once.
Document this on the public boundary and characterize it using public headers.
Do not add implicit retry or alter scheduler behavior.

Preparation currently accepts some finite positive float frame rates whose
existing float-derived duration overflows before conversion to double. Reject
these assets before publication with UnsupportedAsset, owned message and /fr
path. Preserve ordinary timing arithmetic and accepted finite-duration inputs;
do not introduce an arbitrary minimum FPS or switch all timing to double.
Test both primitive and vector profiles, an overflowing normal-positive rate
1e-37 and a finite nearby rate 1e-35; retain the earlier underflow rejection.

## 2. Measurement boundary

The frozen baseline measured 1/4/16 playback entries in **one** controller.
The new scenario adapter measures distinct controller/widget sessions, each
with one playback, on the existing shared executor. It is a versioned extension
of the existing kit, not a diagnostics framework. Reuse observe_process.py,
production host sources and the normal static Release profile. Do not modify
the five frozen files or replace the installed EXE for baseline measurement.

Add only missing cumulative counters to existing diagnostics: preparation
attempts/successes/time; successfully created output QImage storage count and
nominal bytes. Existing device/target/readback counters are sufficient. Counts
are events, not live native memory; Qt allocations, row pitch, driver resources
and allocator reservation are not inferred from nominal bytes.

Scenarios use the existing original Duck think and Party assets and own-authored
20-logical-pixel icon. Fresh serial processes cover 1/4/16 separate canvas
sessions (alternating supported assets, 256x256 physical raster each) and
1/4/16 actual MotionButtonIcon buttons (physical size observed from DPR).
Record empty, created/loaded/first frame, steady, paused/hidden, partial deletion,
all-deleted and one repeat-create/delete phases before process exit. Do not keep
QSignalSpy or image snapshots alive in post-delete memory phases.

Use existing 0.5-second retained-PID observer for CPU, private memory, working set
and handles. Record IDs and counters per element, source/build/input/kit hashes,
DPR, requested/raster/show size and phase times. At least 5 seconds per memory
plateau, 10 seconds steady per independent-element case. These are bounded
exploratory costs, not leak-free or statistical speedup proof.

Separately sample 20 real icon activations when otherwise quiet and 20 while a
distinct Party session renders at 1024x1024, reporting the actual size and its
render-call durations. Name Enter/control-to-first-changed-image and completed
Qt paint separately; neither is physical presentation. Measure GUI owner
deletion during that supported busy rendering, record overlap evidence from
render/publication boundaries and verify the surviving session continues.
No deliberate worker delay is admitted into performance samples. A bounded
safety test may use a controlled barrier and must be labeled as correctness.

## 3. Source-informed selection, not presumed optimization

Pinned lib_lottie fbb922c30b7c30ba17ad5fd4fcb989eadd620062:
[GoodStorageForFrame](https://github.com/desktop-app/lib_lottie/blob/fbb922c30b7c30ba17ad5fd4fcb989eadd620062/lottie/lottie_common.cpp#L65-L73)
checks exact size, premultiplied format and exclusive QImage ownership.
[Icon frame handoff](https://github.com/desktop-app/lib_lottie/blob/fbb922c30b7c30ba17ad5fd4fcb989eadd620062/lottie/lottie_icon.cpp#L301-L361)
uses current/preloaded slots and async state ownership. The useful principle is
bounded reuse only after ownership returns, not overwriting a published frame.
Study ideas only; no upstream code/dependency/artwork is copied.

Selection follows measured attribution: output-buffer churn, repeated asset
preparation, per-element native resources or synchronous deletion may dominate
different scenarios. Compare alternatives in a short result addendum. Choose
one bounded change whose tests and expected observable benefit are explicit.
Before implementing it, append the exact ownership/API design and task plan;
do not pre-authorize an unmeasured cache, device pool or scheduler replacement.
If none is defensibly useful, preserve results and report that conclusion.

## Acceptance

Both public findings have public regressions; original supported profiles still
pass. Independent sessions are measured as independent sessions with device,
prepare and image-storage counts. In-process post-delete memory and contention
latencies are recorded with limitations. The selected single change has a real
RED/GREEN, retained-image and independent-playback tests, independent review and
same-instrument comparison. Fresh no-reference/package and explicit Windows
preview gates are retained; promotion, if needed, is guarded and recoverable.
