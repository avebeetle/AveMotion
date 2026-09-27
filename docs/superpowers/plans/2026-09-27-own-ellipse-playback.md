# Part26I Own Ellipse Playback Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Drive the constrained own ellipse stream from host time, reusing existing first-party playback control without a second scheduler.

**Architecture:** Runtime-private control state/math extracted with behavior-preserving Instance delegation; Rendering-private owning wrapper over H; literal no-reference and independent Telegram mapping/scene proof.

**Tech Stack:** Existing C++20, MSVC2022, CMake/Ninja/CTest; Telegram only for differential tests.

**Spec:** docs/superpowers/specs/2026-09-27-own-ellipse-playback-design.md (3725291).

## Global Constraints

- Direct main and scoped ordinary commits/pushes; root owns ALL Git writes. No force-push, rebase, amend or history rewrite. One product writer; preserve raw/SDD evidence.
- No vendor, dependency, license/notice, fixture or golden changes; no dependency installation or Windows/settings/security/automation actions.
- No Avelabs writes/builds/GUI or UI/out recreation. Preserve its accepted Release and current unrelated work.
- No public loader/API, Runtime handles, Player code/threading, Direct2D ownership, ANGLE/backend coverage or public fallback changes. New interfaces are private/not installed; reference-linked install prohibition stays.
- Own reader/admission/model/stream and old certificate/stream semantics remain unchanged. No larger primitive subset, new corpus, generic producer framework, new scheduler, timer, worker or cache service.
- No full Lottie/SVG, complete host playback, hardware GPU, zero-allocation, TSan or speedup claim. Timing control is functional behavior, not a performance measurement.

## Review focus and execution

Binding risks: unchanged no-reference Instance guards; exact duration float
division vs normalized double mapping; pure snapshot vs success-only Holding;
shared-code comparison is wiring evidence only; H ownership/domain constraints.
Inspect pinned Telegram code at architecture decisions, record applicable ideas
and source, not wholesale copying. No new automation for this workflow checkpoint.

One fresh implementer then independent task review per block. Root owns docs,
ledger and ALL Git writes, workers only return ready-for-commit. Workers never
spawn subagents. Runtime task then own wrapper task: no simultaneous writes or
builds. Reports under this plan's ignored SDD workspace. Raw unique attempts in
out/part26i/task-N; do not overwrite previous evidence. Use VsDevCmd.bat
-arch=x64 -host_arch=x64 before CMake/CTest. Missing API/compiler is not functional
RED: use compiling unsuccessful stubs, observe expected assertions, then implement.

### Task 1: Shared private playback control and unchanged Runtime delegation

**Files:** new src/runtime/PlaybackControl.hpp/.cpp,
tests/playback_control_tests.cpp; modify src/runtime/Runtime.cpp, CMakeLists.txt;
tests/playback_tests.cpp only if needed to pin missing legacy transitions.

**API:** runtime::detail::PlaybackControl retains existing fields/defaults:
status Stopped, direction Forward, loopMode Loop, clip0, playbackRate1,
anchorPosition0, anchorTime0, revision1. Private functions:
clampPlaybackPosition(double), playPlayback(control,now),
pausePlayback(control,duration,now), resumePlayback(control,now),
stopPlayback(control), seekPlayback(control,position,now),
controlPlayback(control,position), setPlaybackDirection(control,direction,duration,now),
setPlaybackRate(control,rate,duration,now)->bool,
setPlaybackLoopMode(control,mode), snapshotPlayback(control,duration,now)->PlaybackSnapshot,
commitPlaybackCompletion(control,snapshot,now).
All noexcept; snapshots leave frameIndex0 for caller mapping. Helper owns no
Runtime/Asset pointer, rendering state, scheduler or reference dependency.

- [x] Write literal table/transition tests against compiling stubs, register
  avemotion.runtime.playback_control in every variant. Show functional RED.
  With duration1s: play0->position.25 at250ms; pause250ms freezes at.25;
  resume500ms->.5 at750ms; direction change at750ms preserves.5;
  reverse at850ms->.4. Stop reverse/play exposes1 exactly at anchor, then.75
  at250ms. Controlled.375 ignores time; resume anchors.375. Once beyond1s
  snapshot Holding/completed but raw control stays Playing/revision stable;
  explicit completion commit enters Holding once; repeated commit is inert;
  play Holding restarts at direction endpoint, resume Holding does not restart.
  Seek Holding becomes Paused. Idempotent play/pause/resume/direction revisions
  and existing non-idempotent setter revisions are pinned.
- [x] Before writing edge tests name the break each catches. Cover negative,
  zero, NaN and infinite rate rejection without mutation; nonfinite/clamped
  position; backward presentation time; INT64_MIN/MAX timestamps without integer
  subtraction overflow; existing invalid-duration/extreme-rate policy. Expected
  literals must not call shared helper code. Use meaningful public outcomes,
  not exact source text or implementation-layout change detectors.
- [x] Implement by moving existing first-party algorithms unchanged; delegate
  Runtime reference-enabled verbs/snapshot/commit. Preserve all no-ref branches,
  frameAtPosition Samsung rounding/large-count fallback and diagnostics.
  No opportunistic policy fix or new thread. Deduplicate real control logic;
  unavoidable no-ref setter branches remain their existing simple semantics.
- [x] GREEN: fresh configure/build/full CTest windows-msvc-direct2d and
  windows-msvc-telegram-debug. Configure/build Samsung and targeted regex
  `avemotion\.(runtime\.(playback_control|playback|frame_mapping)|player\.scheduler)$`.
  Record all warnings/failures by name, no full Samsung green claim.
- [x] Self-review against spec and unchanged Runtime behavior; detailed report
  with raw paths/commands/counts and RED/GREEN. Root verifies focused helper
  plus old playback/Player, commits, independent spec/quality review, then push.

### Task 2: Own time-driven wrapper, literal lifecycle and Telegram wiring

**Files:** new src/render/OwnNativeEllipsePlayback.hpp/.cpp,
tests/own_native_ellipse_playback_tests.cpp,
tests/own_native_ellipse_playback_differential_tests.cpp; CMakeLists.txt.
Reuse unchanged tests/support/OwnNativeEllipseStreamTestData.hpp,
NativeEllipseTestAssets.hpp, OwnNativeEllipseStreamComparison.hpp and ordinary
NativeEllipseOracle.hpp. No H/G production edits or comparator exclusions.

**API:** render::detail::OwnNativeEllipsePlayback factory create(sealed shared
prepared owner), noncopyable/nonmovable; new OwnNativeEllipsePlaybackCreateResult
has OwnNativeEllipseCreateCode code, string message,
unique_ptr<OwnNativeEllipsePlayback> playback and explicit bool. Preserve H
failure code/message. Private wrapper owns one H stream, prepared owner and
PlaybackControl. Expose Instance-shaped verbs, playbackSnapshot(now),
frameAtPosition(position), durationSeconds(), frameRate(),
evaluateAt(now,width,height)->OwnNativeEllipseFrameResult. Durations exactly
double(float(N-1)/floatFrameRate); mapping truncates double normalized*(N-1),
nonfinite->0 and clamp[0,1]. No new validity policy on sealed G input.

- [ ] Write no-ref tests/stubs and register avemotion.own_native_ellipse_playback
  in all variants; retain functional RED. Null factory error, N61/fr60 duration1s,
  literal frames0,15,30,60 for controlled0/.25/.5/1; normalized bounds/nonfinite,
  just-below/above mapping thresholds; actual emitted frame/geometry.
  Play/pause/resume/seek/reverse/loop/once controls use shared helper; own
  evaluateAt on invalid viewport at terminal time must not commit revision;
  subsequent successful terminal emission commits once, earlier pure snapshot
  did not. Successful scenes retain H sequence/change history.
- [ ] Add real JSON and existing TGS preparation-to-playback cases, two objects
  with independent controls/identities sharing prepared owner and one own-only
  planner, retained scene/plan after destruction, planner forget/rebuild. Validate
  invalid Runtime handles/source identity and no cross-object history leakage.
  API comment states H domain/single-writer/allocator lifetime/caller retirement
  contract. Do not add test-only state access to live playback.
- [ ] Implement minimal wrapper and Runtime-private dependency. Compile new
  source in Rendering for all variants, no installed header, no new library.
- [ ] Add Telegram-only avemotion.own_native_ellipse_playback_differential:
  all15 existing variants, four existing viewports, deterministic trace covering
  forward samples, pause/resume, seeking, controlled, rate/direction, reverse
  endpoint and looping, Once completion/failure/retry. Compare full snapshots
  including revision with ordinary Instance under identical operations, matching
  successful evaluateAt commits (pure snapshots cannot substitute). Compare
  duration and mapped frame against directly loaded ordinary Telegram, including
  fractional frame-rate cases and clamped/nonfinite handling at our boundary.
  State explicitly shared-control snapshot differential is wiring-only evidence.
- [ ] For trace emissions sample fresh ordinary scenes independently at mapped
  frames; use H unchanged approved full scene/plan comparison helpers and fixed
  metadata mapping. No candidate-derived semantic expectations. Preserve literal
  tests as independent timeline proof. Demonstrate own calls add no reference
  counter activity; no-ref compile/link is the stronger dependency proof.
- [ ] GREEN full none and Telegram CTest; targeted Samsung new own playback test
  proves no rounding macro leaks. Root focused check, commit, task review/push.

## Final review and gates (root)

- [ ] One whole-stage review from345f1b6 on most capable model, deferred findings
  visible. One combined final fix wave and scoped re-review if needed; do not
  silently drop reviewer limitations.
- [ ] Freeze tested product HEAD; sequential configure/incremental build/full
  CTest none, Telegram and explicit windows-msvc-win32-preview with unique raw
  logs/JUnit. Samsung targeted shared/old/own playback, mapping and scheduler.
  Existing H differential/WARP/lifecycle gates stay unchanged. Verify both
  vendors and TGS integrity through existing verification.
- [ ] Actual include trace of new PlaybackControl.cpp and OwnNativeEllipsePlayback.cpp
  in none/Telegram; generated link/interface/install checks; Runtime.cpp source
  diff preserves guards and no public header/install changes. Retain input hashes,
  exact commands/results. Check host status/out/accepted EXE read-only again.
- [ ] Write Part26I report and update STATE/plan/durable ledger; ordinary guarded
  push, report honest scope and next minimal scheduler/host design. Preserve raw
  and SDD evidence. Do not resume automation or start extra feature work merely
  because this intermediate block completed.

## Plan self-review and approval

Task1 produces one private control interface, Task2 consumes it without touching
its implementation. Tasks share only sequential CMake registration. Spec wins
on conflicts. Scope contains no duplicated scheduler, generic producer or host
switch. Controller selects SDD and approves execution under delegated authority.
