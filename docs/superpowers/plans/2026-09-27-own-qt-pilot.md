# Own Qt Pilot Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Display the existing own animation path in an isolated static Avelabs Motion Lab without rlottie or a second scheduler.

**Architecture:** One compile-time own-warp alternative in the existing MotionWorker. A private host adapter composes existing own preparation and renders existing scenes/plans through host-owned WARP into owned QImages. Default reference mode and accepted Release stay unchanged.

**Tech Stack:** Windows x64, C++20, Qt 6.10.0, MSVC 2022, Direct2D/D3D11 WARP; static app /MT, existing dynamic QtTest /MD.

**Spec:** docs/superpowers/specs/2026-09-27-own-qt-pilot-design.md (9170bc5).

## Global Constraints

- AVELABS_ENABLE_MOTION_LAB stays OFF by default.
- AVELABS_MOTION_LAB_RENDERER accepts reference-cpu (default) or own-warp; own-warp selects none and Direct2D ON, reference-cpu selects telegram and Direct2D OFF.
- One MotionController, MotionWorker, QThread, single-shot QTimer and Player; no new scheduler, registry or fallback.
- No accepted build/Release replacement, install, Avelabs/out creation, dependencies/Windows changes, vendor/golden/corpus changes or GitHub Actions.
- Private engine includes are build-tree-only and PRIVATE; own product must not build/link rlottie or avemotion_reference.
- Preserve docking, tray/Exit, window animation, settings, DPR logic and reference-linked install prohibition.
- Root alone stages/commits/pushes both main branches; workers do not mutate Git state. Preserve raw evidence and SDD.

## Review Focus

- Unsupported or oversized replacement must preserve old usable session and correct generation/count; Task1 actual failed replacement tests.
- Device/readback errors must not publish partial batches or create an automatic retry spin; Task1 failure state and explicit retry test.
- QImage storage must not alias mapped or reused target memory; Task1 retained image after render/resize/destruction.
- Own ScheduledFrame lacks Runtime pointers; Task1 real own controls/count/retirement with no-reference link and actual colored pixels.
- Choosing own must not contaminate default/reference builds or accepted Release; Task1 boundary matrix, Task2 static artifact/hash proof.

## File ownership

All product changes are in D:/rvc/c++/DragonianVoice/Avelabs-UI. AveMotion product
files remain unchanged; root owns this spec/plan/ledger/STATE and final report.
One product task keeps helper, build mode and worker wiring atomic. It contains
several short RED/GREEN cycles, not a helper checkpoint presented as visible UI.

### Task 1: Own renderer and existing Motion Lab integration

**Files (Avelabs):**
- Create src/app/motionlab/OwnMotionRenderer.h/.cpp (private adapter only).
- Modify src/app/motionlab/MotionWorker.h/.cpp, MotionTypes.h, MotionLabPage.cpp.
- Modify cmake/AveMotionLab.cmake, CMakeLists.txt, tests/motionlab/CMakeLists.txt.
- Modify tests/motionlab/tst_motion_worker.cpp, tst_motion_page.cpp, test_build_boundary.py.
- Create tests/motionlab/tst_own_motion_renderer.cpp, own_engine_smoke.cpp and a small test-only gzip helper if needed.
- Create docs/build/own-motion-lab.md with exact isolated commands and scope.
- No other production paths; ask controller about necessary deviations.

**Interfaces:**
- Consumes current OwnNativeEllipsePreparedAsset, OwnNativeEllipsePlayback,
  addOwnNativeEllipsePlayback/ownNativeEllipsePlayback, Player, Tgs formats,
  MotionRenderPlanner and Direct2D Backend. No engine interface changes.
- Produces Avelabs::MotionLab::prepareOwnMotionAsset(const QByteArray&, QString&)
  -> shared_ptr<const avemotion::render::detail::OwnNativeEllipsePreparedAsset>.
- Produces OwnMotionRenderer with pimpl, constructor/destructor,
  QImage render(avemotion::render::detail::OwnNativeEllipsePlayback&,
  avemotion::runtime::MotionTime, QSize, QString&), void reset(),
  OwnMotionRenderDiagnostics diagnostics() const; diagnostics fields
  quint64 deviceCreations, targetRebuilds, readbacks. No new timer/thread.
- Mode macro AVELABS_MOTION_LAB_OWN_WARP PRIVATE to app/test targets. Header
  forward declarations keep D3D/private engine dependencies in implementation.
- Own badge exactly "Experimental — own ellipse / WARP readback"; reference
  badge unchanged. Common error wrapper may retain RuntimeErrorCode wording
  for reference only; own failures identify OwnMotionError and actual stage.

- [x] Step 1: Record clean host BASE712d454 and current engine SHA, protected
  Release hash, no out directory; read current MotionWorker/control/page tests.
  Read spec plus out/part26k-design/{engine-inventory,host-tests}.md as context
  (recommendations there are not requirements; this plan/spec decide).
  Run baseline reference standalone Motion Lab CTest in a fresh
  build/cmake/part26k-reference-tests tree, raw under build/checks/part26k-task1.

- [x] Step 2: Add functional renderer RED tests with a compiling null/error
  stub and own no-reference test target. Use unchanged basic JSON. Assert at
  256x256: transparent corner, blue/cyan interior near (90,128) at position0,
  visible mass moves right near (166,128) at position1; alpha/premultiplication
  and exact QImage format/size. An independently hand-derived pixel checks
  actual output, not the renderer against itself. Save failing raw output.

- [x] Step 3: Implement preparation and WARP adapter. Compose existing parser
  pipeline; metadata width1..4096, frames1..18000, finite positive rate/duration.
  Reject empty/unknown input, malformed UTF8/JSON, unsupported structure and own
  reader limits with actual stage/path; use strict existing decodeTgs limits.
  Use returned HRESULT errors, RAII COM/map ownership, transparent BGRA8 target,
  96 DPI/physical viewport, row-pitch memcpy to owning ARGB32_Premultiplied QImage.
  No test-support process-exit helpers in production. Cache device/target/staging;
  reset planner/backend on session retirement, release broken surfaces/device
  on native failure for explicit retry. Cover same-size reuse counters, resize,
  2 independent playbacks sharing prepared asset, transparent inactive output,
  reset, zero/oversize viewport refusal and image survival beyond renderer.

- [x] Step 4: Add compiling behavioral build-selection RED before changing
  helper/root logic: own-warp has no rlottie target and no reference library in
  app/smoke build/link closure (the existing EXCLUDE_FROM_ALL Reference wrapper
  declaration may remain in the codemodel), D2D exists;
  invalid mode/conflicting variant/own-with-lab-OFF fail; default ON remains
  reference and OFF has no engine. Existing guards/cache-sentinel tests retained.
  Implement selector and shared target-wiring helper in AveMotionLab.cmake only
  if it eliminates repeated PRIVATE source/include/link definitions. Link
  existing Formats/Rendering/Player/Direct2D plus d3d11. Do not add exported target.

- [x] Step 5: Add worker/page functional RED (own file currently cannot produce
  first frame through reference-free Runtime). Wire own preparation, candidate
  registrations, typed snapshots and render at a single sampled tickTime.
  Use existing transport/visibility/generation/pixel-budget logic. Successful
  load/count replacement clears own planner/backend; failed candidate leaves
  active state. On own render error pause all active entries, stop timer and
  report once without publishing incomplete batch; explicit controls can retry.
  Expose actual renderer counters in diagnostics and truthful page labels.

- [x] Step 6: Run all reusable existing worker/page cases in both modes.
  Reference pixel oracle stays reference-only; own counterpart uses literal
  visible/color/movement assertions. Generate temporary TGS from exact basic
  fixture bytes using test-only stored DEFLATE+CRC or existing Python stdlib,
  not a new dependency or committed external asset. Test existing repeater TGS
  as unsupported-own. Reference 2MiB JSON-positive test stays; own explicitly
  tests its 1MiB reader boundary/refusal, never silently skips it. Include
  first image, 1/4/16, pause/seek/stop, hidden pause/Freeze/count replacement,
  resize/aspect, stale load/publication, blocked shutdown and diagnostics bounds.
  Own-render error test can use a valid admitted asset whose finite inputs emit
  nonrepresentable geometry (record reason) rather than adding test-only product
  fail switches. If admission makes that unreachable, use the spec's bounded
  private test-access friend and real Viewport refusal at renderBoundary; do not
  claim native HRESULT injection. An explicit subsequent valid load must recover.

- [x] Step 7: Build own static smoke that runs real prepare/playback/render
  against basic JSON and checks nonempty pixels; no QtTest in static app.
  Keep old reference smoke selected in reference mode. Update tests/CMake,
  build instructions and boundary script for both modes. Preserve supported
  source/CRT/no-install guards; real configure/link behavior, not source grep.

- [x] Step 8: Full tests for each standalone Motion Lab build, serial:
  cmake --build <tree> --config Release --parallel 4;
  ctest --test-dir <tree> -C Release --output-on-failure --no-tests=error -j 1.
  Dynamic PATH C:/vcpkg/installed/x64-windows/bin; explicit Windows plugin path.
  Build target all, not only focused target before full suite. Preserve all
  attempts and QtTest totals/warnings/skips in report. Run real boundary script
  with a new build/checks/part26k-boundary root; no GUI loops or timed workloads.

- [x] Step 9: Self-review and write task report with functional RED/GREEN,
  exact commands, complete result totals, file list, deviations and concerns.
  Root verifies scoped diff/tests and creates normal commit, then fresh task
  reviewer judges spec+quality. Workers never commit or spawn reviewers.

### Task 2: Frozen static/visual acceptance and handoff

**Files:** root-owned docs/PART26K_OWN_QT_PILOT_REPORT.md, STATE and ledger;
Avelabs docs/build/own-motion-lab.md may receive results. No new product scope.

**Interfaces:** consumes reviewed Task1 mode/targets and recorded commands.
Produces honest evidence of static own app, actual UI pixels and preserved base.

- [ ] Step 1: Freeze product SHAs, run independent whole-stage review of
  Avelabs712d454..HEAD and AveMotion docs stage. Resolve findings with at most
  one combined final fix and one scoped re-review, preserve failed attempts.
- [ ] Step 2: Fresh full reference and own standalone QtTest runs; capture own
  page AFTER its loaded animated fixture at two different seek positions.
  View PNGs, verify visible own badge/content and changed position. Qt grab is
  not a native desktop/DPI/hardware acceptance. GUI tests serial, own processes.
- [ ] Step 3: Configure isolated static own app under
  build/cmake/part26k-own-static using existing Qt6.10 /MT dependencies, build
  AvelabsUI and own smoke, run CTest. Preserve configure/build/smoke logs.
  Verify generated app link closure has no rlottie/avemotion_reference, no
  install scripts and no Qt/CRT DLL imports via existing static audit tool.
  Check default OFF configuration still excludes engine; don't build accepted
  normal target. Do not claim static UI runtime from dynamic QtTest evidence.
- [ ] Step 4: Fresh engine full none and explicit windows-msvc-win32-preview
  CTest under VsDevCmd, sequential after host workers finish; no engine source
  delta expected. Preserve exact source hashes rather than attributing tests
  to earlier binaries. No repeated full Samsung gate for host-only change.
- [ ] Step 5: Record measured renderer target/device/readback counters and
  observed render times for bounded test cases, not an A/B speedup. Recheck
  accepted Release SHA, no Avelabs/out, both clean scoped Git diffs/remotes.
  Root updates ledger/STATE/report, normal commits and guarded ordinary pushes.
  If remote changed externally stop only that push. No force/history rewrite.
  Report limited own subset/readback and next measured coverage/output decision;
  no new stage/automation merely to fill time.

## Approval and self-review

Controller reviewed this plan against spec9170bc5 under delegated authority and
selected SDD with one product writer and independent reviews. All five focus
risks have Task1 assertions or Task2 artifact proof. No design-only checkpoint
substitutes for the requested own UI output. Raw/SDD retained by user direction.
