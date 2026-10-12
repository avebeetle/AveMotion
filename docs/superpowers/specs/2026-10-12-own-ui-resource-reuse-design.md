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

## Measured-selection addendum — 2026-10-12

Accepted A is H `build/checks/2026-10-12-motion-elements-baseline-002/series-A`,
compiled from E a596884/H1059ac0. All eight serial full cases passed unchanged
strict analysis; the rejected baseline-001 stays separate. The report is H
`docs/testing/motion-elements-2026-10-12/REPORT.md`. It measures a separate static
production-linked probe, not the installed shell. Each independent session
prepared its asset and created its device/target once. Canvas16 steady10s had
9590 successful output-image allocations/readbacks, 2513960960 nominal bytes.
Busy icon paint p95 was19.7329ms (20 activations); one actual overlapping owner
deletion took1.307ms with peer survival. These are exploratory observations.

Select exactly one change: renderer-local, two-slot output QImage reuse.
Preparation sharing addresses one-time work (canvas16 aggregate130.23ms), not
steady churn. Device sharing would cross lifetimes without native-memory
attribution. One deletion sample does not establish a retirement bottleneck.
Do not bundle these alternatives with output storage reuse.

Only H `src/app/motionlab/OwnMotionRenderer.cpp` production implementation and
`tests/motionlab/tst_own_motion_renderer.cpp` regressions change. No public
signature, worker, controller, Player, executor, resource manager or format
change. `Impl` owns exactly two QImage anchors and a bounded replacement cursor.
Private helpers are `QImage takeFrameStorage(QSize size, size_t& slot)` and
`void clearFrameStorage()`; publication restores the selected anchor only after
all rows are filled. All access remains on the existing renderer owner thread.

Storage is reusable only at the exact fitted target size, with
Format_ARGB32_Premultiplied and isDetached()==true. Move/swap the eligible anchor
into a sole local owner BEFORE writable scanLine access, avoiding implicit-copy
detach. If none is eligible, create fresh QImage storage and replace one bounded
anchor; consumers retain the old pixels through Qt ownership. No consumer-held
image may be mutated, even when both slots are occupied or playback is reset.
Successful fresh constructions alone increment the existing allocation/nominal
byte counters. Reuse does not increment them; readbacks still count each output.

Clear anchors/cursor on target resize, reset and native failure. Native devices
retain their existing reset lifetime. Invalid-viewport or evaluate/plan errors
still return explicit errors and never mutate retained images. Existing WARP
draw, full row-copy/Map, transparency, fitting and pixel quality remain unchanged.
Each renderer may retain at most two1024-square output anchors (8MiB nominal);
external retained images are NOT bounded by those anchors. Smaller sizes retain
their exact-sized storage only. This is a small working set, not a frame cache.

Proof: twelve sequential256-square renders, with the current consumer image
retained until replacement returns, use exactly two successful allocations;
retain multiple earlier frames to force fresh fallback and check byte stability;
resize/reset/destruction and two independent renderers preserve prior pixels
and playback. Keep odd-row, premultiplied-edge, inactive-clear and original
Duck/Party assertions. No tests or goldens are loosened. Native failure cleanup
is inspected in the shared cleanup path; no new fault-injection API is added.

Compare frozen A against a clean committed candidate B with identical13-file
adapter/observer hashes, eight cases, physical sizes and inputs. Benefit is fewer
actual storage events/nominal allocation traffic at comparable readback cadence,
not a promised CPU/private-memory/latency improvement or zero-allocation result.
Report all memory/latency tradeoffs, including post-delete/repeat phases and
one-sample busy deletion. Keep the candidate only if correctness holds and
allocation reduction is observed without an unexplained behavioral regression.
Standing delegated written design approval applies; root records preflight and
independent report review before implementation. Native icon acceptance stays open.
