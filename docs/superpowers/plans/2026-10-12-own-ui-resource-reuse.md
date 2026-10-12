# Own UI resource costs and bounded reuse Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Harden two public contracts, measure separate UI sessions, then select one evidence-backed bounded reuse improvement.

**Architecture:** Keep the existing public own API, Player and shared executor. Extend existing diagnostics and its measurement-only adapter; select implementation only after baseline attribution, with a concrete addendum before writing improvement code.

**Tech Stack:** Windows x64, MSVC, C++20, Qt Widgets 6.10.0, Direct2D WARP, Python 3.9 standard library.

**Spec:** docs/superpowers/specs/2026-10-12-own-ui-resource-reuse-design.md

## Global Constraints

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

## Review Focus

- Failed terminal scene request: no false committed revision, no implicit retry; public explicit invalidation succeeds (Task 1).
- Tiny normal-positive fr: reject only nonfinite duration, not finite slow playback (Task 1).
- Replaced/failed loads and size/count controls: preparation counters count actual calls, not playback entries (Task 2).
- Retained images: instrumentation does not detach/mutate them or keep probe snapshots during deletion (Task 2).
- Busy shared executor: latency endpoints distinguish command, image, paint and deletion, with real overlap and surviving-session checks (Tasks 2–3).

## Task 1: Public completion contract and duration admission

**Files:** E/include/avemotion/render/OwnPlayback.hpp; E/src/render/OwnPlayback.cpp; E/tests/own_public_playback_tests.cpp.

**Interfaces:** Existing prepareOwnAsset(span<const byte>), createOwnPlayback(shared_ptr<const OwnAsset>), addOwnPlayback(Player&, shared_ptr<OwnPlayback>, MotionTime, PlayerEntryOptions), Player::invalidate(PlayerHandle), OwnPlayback::evaluateAt(MotionTime,size_t,size_t). No new public signature.

- [ ] Add public-only terminal characterization: Once/play at 0, terminal tick at 2s for the 61-frame fixture, failed zero-width evaluation returns InvalidViewport and unchanged revision; next tick has no frame; invalidate yields ExplicitInvalidation; success emits frame60 and increments revision exactly once; repeat terminal evaluation does not increment again. This is pre-GREEN existing behavior, not a fabricated functional RED.
- [ ] Add public preparation regression by replacing fixture fr with 1e-37 for primitive and vector fixtures; require no asset, UnsupportedAsset, owned nonempty message and path /fr. Require fr1e-35 accepted with finite positive duration and successful initial evaluation. Keep underflow fr1e-307 and existing rejection tests.
- [ ] Build existing no-reference package-build Release public target; run ctest -C Release -R own_public_playback --output-on-failure --no-tests=error. Preserve actual expected functional RED for overflowing duration before production edits; setup failures do not count.
- [ ] Guard the existing float-derived duration in prepareOwnAsset before publication; preserve ordinary arithmetic. Document projected completion and explicit retry on public snapshot/evaluate/Player handoff comments.
- [ ] Rebuild and run focused test, then complete package-build Release suite and explicit windows-msvc-win32-preview suite. Record all warnings/failures rather than hiding them.
- [ ] Independent spec/quality review; root fresh focused verification, STATE/ledger update, scoped commit and ordinary push after remote ancestry check.

## Task 2: Minimal counters and independent-element scenario adapter

**Files:** H/src/app/motionlab/{MotionTypes.h,MotionWorker.h,MotionWorker.cpp,OwnMotionRenderer.h,OwnMotionRenderer.cpp}; H/tests/motionlab/{tst_motion_worker.cpp,tst_own_motion_renderer.cpp}; create H/tools/diagnostics/motion-elements-v1/{README.md,protocol.json,probe/CMakeLists.txt,probe/measure.cpp,collect.py,analyse.py}; create H/tests/diagnostics/test_motion_elements.py. Do not modify frozen motion-baseline five files or observe_process.py.

**Interfaces:** Append diagnostics ownAssetPrepareAttempts/ownAssetPrepareSuccesses/ownAssetPrepareNanoseconds and ownFrameStorageAllocations/ownFrameStorageAllocatedBytes (cumulative per session). Renderer diagnostics supplies frameStorageAllocations/frameStorageAllocatedBytes; worker counters wrap only actual prepareOwnMotionAsset calls. No GUI/controller access API: probe finds the existing QObject child MotionController of each MotionButtonIcon.

- [ ] Write real tests: two independent controllers loading same supported fixture each prepare once/create one device; setInstanceCount/repaint does not prepare; explicit reload increments its session only; rejected preparation increments attempts not successes. Renderer two successful renders allocate two distinct output storages/nominal bytes and leave retained first pixels unchanged. New counter absence is a compile RED; if behavioral assertion fails, retain it before production code.
- [ ] Add counters at successful allocation/actual preparation call sites; expose via existing diagnostics. Do not add a new observer, resource ownership or production scheduler.
- [ ] Derive measurement-only CMAKE_PROJECT_INCLUDE hook from existing baseline pattern: target avelabs_elements_probe, same static profile, production sources except main, no install rule, compiled E/H identity and input hashes. Separate-element probe uses actual MotionCanvas/controller or MotionButtonIcon/button, one playback each. Use QObject child discovery instead of production test accessors.
- [ ] Implement versioned protocol: separate canvases1/4/16 alternating Duck/Party, icons1/4/16, memory plateaus5s, steady10s, pause/hide5s each, partial/all deletion5s each, one repeat create/delete. Fixed canvases256-square physical; icon20logical with observed DPR. Bound telemetry, release images before post-delete samples, preserve element IDs and counters.
- [ ] Add quiet/busy icon20activations; busy separate Party1024-square. Measure first changed frameReady after Enter and button completed Paint separately. Measure deletion initiated after a real render boundary, retain publication timestamps to establish overlap, require surviving frames. Do not sleep/stall worker for timing.
- [ ] Thin collector reuses observe_process.py at0.5s, retained childPID, serial cases, exclusive output paths and hashes. Analysis reports raw/derived CPU/private/working-set/handle plateaus and named latency quantiles; mismatched sizes/hash/empty samples/incomplete phases fail, not silently pass. Test parser/analysis on controlled telemetry and failure inputs before implementation.
- [ ] Run H diagnostics suite and own motion6test suite once, static measurement probe build; independent task review before root scoped commit/push. Preserve old kit hashes. Do not yet optimize or promote EXE.

## Task 3: Baseline evidence and exact single-improvement decision

**Files:** create H/docs/testing/motion-elements-2026-10-12/REPORT.md; add a measured-selection addendum to the spec and concrete implementation task(s) to this plan; update E STATE and this plan's ignored ledger.

**Interfaces:** Task2 adapter/protocol/collector plus unchanged observe_process.py; E/H source heads, frozen kit hashes and raw evidence directory. Produces baseline source/EXE/kit/input identities and one justified selected ownership change (or evidence that none is justified).

- [ ] Verify no foreign AvelabsUI/process, build/test workers or executor runs overlap timing. Stop only affected measurement if occupied; do not kill foreign processes. Freeze baseline EXE/PDB/adapter copies and their hashes before improvement code.
- [ ] Run all six resource cases and quiet/busy/delete cases serially, with unique raw receipts. Record exact commands and in-process deletion samples; retained allocator memory is not automatically a leak.
- [ ] Analyse counts/attribution/latency; distinguish initial prep, devices, frame-storage churn, live memory plateaus and GUI wait. Keep negative or inconclusive results.
- [ ] Compare bounded alternatives (per-renderer reusable output storage, explicit asset sharing, native-resource sharing, retirement responsiveness) against measured costs and ownership risk. Select exactly one useful change, not a bundle; no large cache/scheduler/universal manager.
- [ ] Write its concrete spec addendum and task with exact files/signatures/RED assertions before implementation. Root delegated approval and preflight table recorded. This gate is an evidence-dependent plan extension, not an implementer instruction to guess.
- [ ] Commit/push baseline report and checkpoint after independent report/protocol review. Continue to the selected implementation and same-instrument comparison; do not stop merely because baseline is an intermediate result.

## Follow-through gate

The selected task must preserve independent controls, immutable retained images,
transparent pixels, async preparation and no-new-work hidden/paused behavior.
It must carry RED/GREEN, independent review and same-adapter before/after runs.
Final verification includes complete none/package suite, explicit
windows-msvc-win32-preview, H own/reference/shell suites where applicable, static
package/import/identity/SymChk if promoted, and one strongest whole-stage review.
No speculative improvement implementation is authorized by an empty task here;
Task3 adds the exact bounded argument from measured evidence.
