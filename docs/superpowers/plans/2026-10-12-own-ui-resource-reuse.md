# Own UI resource costs and bounded reuse Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Harden two public contracts, measure separate UI sessions, then select one evidence-backed bounded reuse improvement.

**Architecture:** Keep the existing public own API, Player and shared executor. Extend existing diagnostics and its measurement-only adapter; select implementation only after baseline attribution, with a concrete addendum before writing improvement code.

**Tech Stack:** Windows x64, MSVC, C++20, Qt Widgets 6.10.0, Direct2D WARP, Python 3.9 standard library.

**Spec:** docs/superpowers/specs/2026-10-12-own-ui-resource-reuse-design.md

**Status (2026-10-12):** Tasks 1–5 complete with stated measurement and acceptance
limits. Reviewed exact E92e1cfa/Haa7474e static candidate promoted at
2026-10-12T08:51:26.0378228Z; installed package/readiness/SymChk exit0. The
comparison is a separate production-linked probe, not installed-shell timing;
native new-icon and mixed-monitor DPR acceptance remain OPEN. Preserve frozen
raw/SDD evidence. No GPU/format/scheduler follow-on is started by completion.

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

- [x] Add public-only terminal characterization: Once/play at 0, terminal tick at 2s for the 61-frame fixture, failed zero-width evaluation returns InvalidViewport and unchanged revision; next tick has no frame; invalidate yields ExplicitInvalidation; success emits frame60 and increments revision exactly once; repeat terminal evaluation does not increment again. This is pre-GREEN existing behavior, not a fabricated functional RED.
- [x] Add public preparation regression by replacing fixture fr with 1e-37 for primitive and vector fixtures; require no asset, UnsupportedAsset, owned nonempty message and path /fr. Require fr1e-35 accepted with finite positive duration and successful initial evaluation. Keep underflow fr1e-307 and existing rejection tests.
- [x] Build existing no-reference package-build Release public target; run ctest -C Release -R own_public_playback --output-on-failure --no-tests=error. Preserve actual expected functional RED for overflowing duration before production edits; setup failures do not count.
- [x] Guard the existing float-derived duration in prepareOwnAsset before publication; preserve ordinary arithmetic. Document projected completion and explicit retry on public snapshot/evaluate/Player handoff comments.
- [x] Rebuild and run focused test, then complete package-build Release suite and explicit windows-msvc-win32-preview suite. Record all warnings/failures rather than hiding them.
- [x] Independent spec/quality review; root fresh focused verification, STATE/ledger update, scoped commit and ordinary push after remote ancestry check.

## Task 2: Minimal counters and independent-element scenario adapter

**Files:** H/src/app/motionlab/{MotionTypes.h,MotionWorker.h,MotionWorker.cpp,OwnMotionRenderer.h,OwnMotionRenderer.cpp}; H/tests/motionlab/{tst_motion_worker.cpp,tst_own_motion_renderer.cpp}; create H/tools/diagnostics/motion-elements-v1/{README.md,protocol.json,probe/CMakeLists.txt,probe/measure.cpp,collect.py,analyse.py}; create H/tests/diagnostics/test_motion_elements.py. Do not modify frozen motion-baseline five files or observe_process.py.

**Interfaces:** Append diagnostics ownAssetPrepareAttempts/ownAssetPrepareSuccesses/ownAssetPrepareNanoseconds and ownFrameStorageAllocations/ownFrameStorageAllocatedBytes (cumulative per session). Renderer diagnostics supplies frameStorageAllocations/frameStorageAllocatedBytes; worker counters wrap only actual prepareOwnMotionAsset calls. No GUI/controller access API: probe finds the existing QObject child MotionController of each MotionButtonIcon.

- [x] Write real tests: two independent controllers loading same supported fixture each prepare once/create one device; setInstanceCount/repaint does not prepare; explicit reload increments its session only; rejected preparation increments attempts not successes. Renderer two successful renders allocate two distinct output storages/nominal bytes and leave retained first pixels unchanged. New counter absence is a compile RED; if behavioral assertion fails, retain it before production code.
- [x] Add counters at successful allocation/actual preparation call sites; expose via existing diagnostics. Do not add a new observer, resource ownership or production scheduler.
- [x] Derive measurement-only CMAKE_PROJECT_INCLUDE hook from existing baseline pattern: target avelabs_elements_probe, same static profile, production sources except main, no install rule, compiled E/H identity and input hashes. Separate-element probe uses actual MotionCanvas/controller or MotionButtonIcon/button, one playback each. Use QObject child discovery instead of production test accessors.
- [x] Implement versioned protocol: separate canvases1/4/16 alternating Duck/Party, icons1/4/16, memory plateaus5s, steady10s, pause/hide5s each, partial/all deletion5s each, one repeat create/delete. Fixed canvases256-square physical; icon20logical with observed DPR. Bound telemetry, release images before post-delete samples, preserve element IDs and counters.
- [x] Add quiet/busy icon20activations; busy separate Party1024-square. Measure first changed frameReady after Enter and button completed Paint separately. Measure deletion initiated after a real render boundary, retain publication timestamps to establish overlap, require surviving frames. Do not sleep/stall worker for timing.
- [x] Thin collector reuses observe_process.py at0.5s, retained childPID, serial cases, exclusive output paths and hashes. Analysis reports raw/derived CPU/private/working-set/handle plateaus and named latency quantiles; mismatched sizes/hash/empty samples/incomplete phases fail, not silently pass. Test parser/analysis on controlled telemetry and failure inputs before implementation.
- [x] Run H diagnostics suite and own motion6test suite once, static measurement probe build; independent task review before root scoped commit/push. Preserve old kit hashes. Do not yet optimize or promote EXE.

## Task 3: Baseline evidence and exact single-improvement decision

**Files:** create H/docs/testing/motion-elements-2026-10-12/REPORT.md; add a measured-selection addendum to the spec and concrete implementation task(s) to this plan; update E STATE and this plan's ignored ledger.

**Interfaces:** Task2 adapter/protocol/collector plus unchanged observe_process.py; E/H source heads, frozen kit hashes and raw evidence directory. Produces baseline source/EXE/kit/input identities and one justified selected ownership change (or evidence that none is justified).

- [x] Verify no foreign AvelabsUI/process, build/test workers or executor runs overlap timing. Stop only affected measurement if occupied; do not kill foreign processes. Freeze baseline EXE/PDB/adapter copies and their hashes before improvement code.
- [x] Run all six resource cases and quiet/busy/delete cases serially, with unique raw receipts. Record exact commands and in-process deletion samples; retained allocator memory is not automatically a leak.
- [x] Analyse counts/attribution/latency; distinguish initial prep, devices, frame-storage churn, live memory plateaus and GUI wait. Keep negative or inconclusive results.
- [x] Compare bounded alternatives (per-renderer reusable output storage, explicit asset sharing, native-resource sharing, retirement responsiveness) against measured costs and ownership risk. Select exactly one useful change, not a bundle; no large cache/scheduler/universal manager.
- [x] Write its concrete spec addendum and task with exact files/signatures/RED assertions before implementation. Root delegated approval and preflight table recorded. This gate is an evidence-dependent plan extension, not an implementer instruction to guess.
- [x] Commit/push baseline report and checkpoint after independent report/protocol review. Continue to the selected implementation and same-instrument comparison; do not stop merely because baseline is an intermediate result.

## Follow-through gate

The selected task must preserve independent controls, immutable retained images,
transparent pixels, async preparation and no-new-work hidden/paused behavior.
It must carry RED/GREEN, independent review and same-adapter before/after runs.
Final verification includes complete none/package suite, explicit
windows-msvc-win32-preview, H own/reference/shell suites where applicable, static
package/import/identity/SymChk if promoted, and one strongest whole-stage review.
No speculative improvement implementation is authorized by an empty task here;
Task3 adds the exact bounded argument from measured evidence.

## Task 4: Two-slot exclusive output-storage reuse

**Files:** H/src/app/motionlab/OwnMotionRenderer.cpp; H/tests/motionlab/tst_own_motion_renderer.cpp only. Root owns STATE/ledger and report checkpoint separately.

**Interfaces:** Existing render(OwnPlayback&, MotionTime, QSize, QString&) -> QImage, reset() and diagnostics() unchanged. Private Impl helpers QImage takeFrameStorage(QSize size, size_t& slot), void clearFrameStorage(); exactly two QImage anchors and bounded cursor. Public engine/playback and all diagnostic meanings remain unchanged.

- [x] Add `reusesExclusiveOutputStorageWithCurrentFrameRetained`: twelve256-square successful renders, alternate playback seek0/1, hold current image until render replacement returns, readbacks12, allocation events2, nominal bytes524288; verify alternating known ink/transparent pixels. Build existing own renderer target and run only this method BEFORE production change; expect functional FAIL allocations12 versus2, not a setup failure.
- [x] Add `retainedOutputSlotsFallBackWithoutMutation`: keep two early distinct frames and byte snapshots alive during later frames; require fresh fallback while all anchors are shared and both snapshots stable. Preserve existing reusesSurfacesAndOwnsEarlierImages assertions. Add exact resize/reset cold-allocation increments, recovery after invalid viewport, and retained-image-after-destruction checks without weakening prior odd-row/premultiplied/inactive tests.
- [x] Add `independentRenderersReuseWithoutCoupling`: two public own playback/renderer pairs; only A seeks/renders/resets/reuses, B position/image/diagnostics unchanged; B can then render independently. No mock storage/scene behavior or production test access API.
- [x] Implement the private two-slot policy from spec addendum. Eligible exact-size/premultiplied/isDetached anchor moves out before writable scanLine; fresh fallback increments allocation counters only on successful construction; fully filled local image is anchored before implicit-sharing return. Clear anchors on clearSurfaces/nativeFailure and reset; keep native reset ownership/row-copy/render path unchanged.
- [x] Run focused new renderer methods, then build/run existing H own suite serially: cmake --build build/cmake/supported-own-ui-task4-own-tests --config Release --parallel 4; ctest --test-dir build/cmake/supported-own-ui-task4-own-tests -C Release --output-on-failure --no-tests=error. Use existing dynamic Qt environment from Task2 receipt; six test executables pass. Record warnings and named external skips. No timing overlaps tests/builds.
- [x] Retain commands/RED/GREEN/source diff in NEW H/build/checks/2026-10-12-motion-output-reuse-task4-001 and write task-4-report.md in this plan's ignored SDD workspace. Independent task spec/quality review, root fresh focused verification, STATE/ledger and scoped two-file commit/ordinary push. Do not promote or modify measurement kit.

## Task 5: Same-instrument comparison and guarded handoff

**Files:** H/docs/testing/motion-elements-2026-10-12/REPORT.md (append comparison); existing H docs/build/current-release.md and docs/ROADMAP.md only if promoted; E STATE/ledger. No new production subsystem or diagnostic tool.

**Interfaces:** Unchanged frozen13-file Task3 adapter/observer; Task4 counters count only actual allocations; A identities/raw evidence remain immutable. Clean committed B with same assets/physical sizes. Existing package/import/readiness/SymChk/promotion contracts.

- [x] After Task4 review/commit, verify E/H clean/current remotes; build fresh own-static Release probe in build/diagnostics/motion-elements-candidate-001 with the same configure options as A. Freeze EXE/PDB/cache/manifest and exact13 kit files in NEW build/checks/2026-10-12-motion-elements-candidate-001. Verify A/B kit hashes equal and unchanged installed EXE/PDB.
- [x] Stop no foreign process; verify no active build/test/GUI overlaps, record idle evidence. Run all eight cases serially using collect.py into series-B, then strict analyse.py. A stays baseline-002/series-A. Root owns timing; no implementer/reviewer GUI overlap.
- [x] Append comparable prep/device/target/storage/readback counts, load, phase memory/CPU/handles, quiet/busy latency and actual deletion overlap/survival to H report. Report adverse results; single A/B is not a statistical speedup test. Reduction in storage events is required; no CPU/private-memory/latency guarantee. If gates fail, establish cause and bounded revision, not weakened checks.
- [x] Fresh final no-reference package46 and explicit windows-msvc-win32-preview97, H own6/reference4/shell3 where applicable, nine original Duck/Party opt-in gates and static2 plus three original smokes. Preserve actual warnings/setup failures and don't call external skips original acceptance. No goldens/formats/scheduler/GPU changes.
- [x] One strongest whole-stage review over E/H start-to-final changes with reports/ledger and deferred warnings; one combined final fix wave plus scoped re-review if needed. Rulings and residual limits recorded. No duplicate task suites without an amended-code reason.
- [x] Prepare normal static /MT AvelabsUI candidate without engine install; package/import/readiness/SymChk verify exact compiled source and notices. Check no running installed app, make verified full-package/PDB backup, guarded promote into same build/Release and build/symbols; verify installed hashes and repeat readiness. Do not close foreign processes or replace a running EXE. Native new-icon visual/mixed-DPR acceptance remains explicitly open.
- [x] Final STATE/ledger/report scoped commit and ordinary push after remote check. Handoff exact SHAs/compiled identities/commands/results/all ordered rulings and limits. Preserve raw/SDD/frozen evidence; do not create schedules or start a GPU/format follow-on.
