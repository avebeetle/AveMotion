# Own Duck Playback Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Play unchanged Duck think through own AveMotion in canonical AvelabsUI.

**Architecture:** Bounded own JSON compiler populates the existing canonical
model. One draw-binding program feeds the existing evaluator/stream/Player and
host WARP route; no duplicate animation runtime or reference product fallback.

**Tech Stack:** C++20, existing CMake/MSVC/CTest, Qt6.10 static /MT on Windows.

**Spec:** `docs/superpowers/specs/2026-09-28-own-duck-playback-design.md`.

Controller-approved under delegated authority; execution method SDD, one product
writer at a time. Root performs all Git operations after tests/review. No routine
approval waiting, no new automation. Baseline engine5936713, host0d56e83 plus
the preserved22-file M delta; Task1 explicitly accepts that delta before reuse.

## Global Constraints

- Unchanged target TGS SHA-256 `7CD55718288EB1B1D1EDFD0E774D9038E076A7F5AADF9835A92CF942948A4BC1`; local evaluation only, never commit/bundle artwork.
- No rlottie/Reference or raster fallback in the product; reference only in tests. Preserve public fallback policy and reference-linked installation prohibition.
- One existing Player/worker/timer/canvas and one own stream emitter; no backend ownership or unrelated UI changes.
- Default JSON limits stay 1 MiB/4096 values/depth32; explicit vector limits 1 MiB/65536 values/depth32.
- Vector compiler limits:128 expanded layers,128 paths,256 draws,256 vertices/path,256 keys/property,4096 properties,8192 segments,65536 stored canonical shape points.
- Preserve exact primitive numeric gates, strict legacy APIs, single-writer identity/history and lifetime contracts. Errors publish no partial owner/frame.
- Root-only scoped commits/ordinary pushes directly in main; preserve raw/SDD. No force/rebase, dependencies/Windows changes, vendor/golden edits or new licensing decisions.
- Canonical L EXE stays unchanged until final verified candidate/backup/promotion. Host artifacts only under build, host out absent; no user-process termination.

## Review Focus

- Parent cycles/duplicate IDs or an opacity-zero null: reject invalid graphs, but keep valid visible children; compiler and stream witnesses in Tasks2/3.
- Last key and held rotation at15/180: no premature interpolation or terminal-value loss; Task2 canonical evaluation and Task3 frame179/loop tests.
- Unknown visual fields or nonidentity precomp clocks: actionable fail-closed rejection, never silently simplify; Task2 mutation tests.
- Static geometry with moving layer/group and reused streams: no stale cached geometry, opacity, trim or device resources; Task3 seek/viewport/retirement and Task4 worker tests.
- Candidate success with stale/mismatched EXE/PDB or accidental reference linkage: actual file/link hashes and recoverable promotion; Task4 artifact checks.

## File responsibilities and interfaces

Existing private historical OwnNativeEllipse names stay to avoid broad renaming.
`OwnVectorModel` owns canonical authored tables and binding IDs, not a second
numeric timeline. `OwnSceneProgram` owns immutable layer/draw instructions over
those tables. Shared `SourcePathMaterializer` owns pure evaluated path conversion.
Current primitive factory remains available; new host entry returns the same
prepared owner. Additional task-local test support stays under tests/support.

### Task 1: Reconcile the preserved multi-resource prerequisite

Execution outcome: complete, independently Approved across all22 carried files.
Full none45/45, TG96/96; root focused4/4. See durable ledger/raw report for
failures and historical warning qualification. Steps below retain design intent.

**Files:** the22 preserved files enumerated in
`.superpowers/sdd/2026-09-27-own-primitives-profile/task-2-report.md`, especially
`src/render/OwnNativeEllipsePreparedAsset.cpp`,
`tests/own_primitive_differential_tests.cpp`,
`tests/support/OwnPrimitiveSceneComparison.hpp`. No host work.

**Interfaces:** preserve current `buildOwnNativeEllipseModel(document)`,
`prepareOwnNativeEllipseAsset(owner)` and `OwnNativeEllipseStream::create/emit`.
The product/test delta from M is included in this task's independent review.

- [ ] Run the existing functional failing differential before production edits:
  MSVC wrapper from `out/part26m/task2/differential-focused.cmd`, output to new
  `out/part26n/task1/001-red.log`. Expected actual failure at multi-item
  `canonicalPaint.sourceKey`, not compiler failure. Read ordinary AssetModelBuilder
  and own preparation to establish the cause; inspect geometry as well as paint.
- [ ] Fix canonical source keys to the resource's model ID+1, preserving singleton
  1 and all old literal hashes. Do not change the comparator to erase IDs or
  remove fields. Add a later-item geometry/paint identity regression if existing
  semantic differential does not protect both. Follow TDD for any other defect
  exposed after the first failing assertion; preserve every failure.
- [ ] Run all61 forward/reverse fixture frames and overlap/seek/viewport/mutation
  witnesses in `own_primitive_differential`; all must actually execute. Keep
  existing one-item model/resources/stream/playback/binding checks intact.
- [ ] Configure/build/full CTest `windows-msvc-direct2d` and
  `windows-msvc-telegram-debug` in VS environment, parallel4 build/serial tests.
  Report counts/warnings/failures. Do not add the superseded M host/promotion
  task or a separate seven-primitive acceptance stage.
- [ ] Report TDD, files, raw logs and remaining concerns; independent review of
  the entire carried delta and fix, root focused verification, update ledger/
  STATE, scoped commit `fix: reconcile own multi-resource scene identities`,
  ordinary push after remote check. No unreviewed M code slips into a later task.

### Task 2: Compile the real vector subset into canonical authored data

Execution outcome: complete, task review and scoped fix1 accepted. Functional
REDs retained, exact neutrality/equality corrected before float conversion.
Final none46/46,TG98/98; root3/3 plus unchanged original TGS inventory and186
ordinary property samples, max3.05176e-05. Not rendering/UI acceptance. The
steps below retain implementation intent; durable ledger/SDD have exact evidence.

**Files:** create `src/runtime/OwnVectorModel.hpp/.cpp`,
`src/runtime/OwnVectorPropertyCompiler.hpp/.cpp` (typed keyframe conversion),
`tests/own_vector_model_tests.cpp`, `tests/own_vector_model_differential_tests.cpp`,
`tests/support/OwnVectorTestData.hpp`, first-party fixtures in
`tests/fixtures/own_vector/`; modify `src/formats/OwnJsonReader.hpp/.cpp`,
`tests/own_json_reader_tests.cpp`, `CMakeLists.txt`.

Approved implementation clarification: also modify private
`src/runtime/NativeEllipseAdmissionCore.hpp/.cpp` to share the existing exact
numeric-token validation/conversion mechanics; no legacy behavior change.

**Interfaces:** add
`OwnJsonReadResult readOwnJson(std::string_view, OwnJsonReadLimits)` with limits
fields `maxValues` and `maxDepth` default4096/32 and hard maxima65536/32; byte cap
unchanged. Invalid zero/out-of-range limits return ResourceLimit without parse.
Keep old one-argument overload. Add private
`OwnVectorModelResult buildOwnVectorModel(const OwnJsonDocument&)` and
`bool validateOwnVectorModel(const OwnVectorModel&)`.
Result has `prepared`, error `path/message`, explicit bool; failed result has
null owner. Owner private construction, `exactJson`, const canonical `model`,
layer/draw binding vectors exposed read-only. Layer bindings carry source layer,
structural parent and intersected interval; draws carry layer/group/path/paint
source IDs and optional trim. No render resource IDs until Task3.

- [ ] Add functional RED for explicit bounded reading/admission of the unchanged
  original TGS and a first-party path layer with opacity-zero transform parent.
  Use original production TGS decode, not substituted decoded JSON. A minimal
  nonworking new API stub is allowed to observe assertion failure, but compile
  errors are not RED. Test executable accepts `--tgs <absolute-file>`; fail if
  the explicitly supplied file is absent/unreadable. Ordinary CTest runs use
  committed first-party fixtures; report real-target invocation separately.
- [ ] Add explicit reader profile retaining old malformed/duplicate/resource
  precedence. Boundary tests show a >4096 valid object rejected by default and
  accepted explicitly; over65536 and depth33 rejected, duplicate UTF-8 keys and
  invalid limits fail, decoded ownership survives input destruction.
- [ ] Compile the spec subset, using canonical PropertyEvaluator types and
  existing numeric conversion mechanics. Split tree/topology and typed property
  conversion by the two named source files; no vendor parser code copy. Validate
  strict contextual keysets, finite/domain/integer values, root metadata and
  bounded graph expansion. Structural/transform parents remain distinct.
- [ ] TDD first-party tests assert hand-derived values: visible child with null
  parent opacity0; position/anchor/scale/rotation order; spatial tangent changes
  the midpoint; hold stays0 at14, becomes30 at15; terminal value retained after
  last key; morph relative controls and closure; outer trim binding targets its
  nested path; fills/strokes draw in authored reverse paint order. Bounds and
  malformed/cyclic/missing graph, unequal ease axes, masks/nonzero skew/time
  stretch, mismatched shape topology, unknown semantic key all reject atomically.
- [ ] Differential uses test-only pinned Telegram ordinary parsed model and
  property evaluation by semantic roles, not flattened raw IDs. Explain the
  observed303-vs304 segment difference; test its evaluated boundary behavior.
  Actual Duck must compile and all180 property samples compare within spec
  limits. Record 36 children/32paths/36painted draws/7trims/33animated properties
  where roles are comparable; count changes from canonical defaults are explained.
- [ ] Run focused reader/model tests through RED/GREEN, then full none/Telegram
  CTest once on final product code. Independent task review, root focused check,
  update STATE/ledger, scoped commit `feat: compile bounded own vector stickers`,
  ordinary push. Duck compilation is not UI playback completion.

### Task 3: Emit own vector scenes through the existing stream

Execution amendment: spec rulings14–16 add a fail-closed precomp clip-equivalence
guard. Preserve the real27 square gate and disclose direct rectangular Duck
rejections; the real rectangular UI gate is satisfied through Task4's explicit
composition-aspect target contract, not a new geometric stroker approximation.

**Files:** create `src/render/OwnSceneProgram.hpp/.cpp`,
`src/render/SourcePathMaterializer.hpp/.cpp`, `tests/own_vector_stream_tests.cpp`,
`tests/own_vector_stream_differential_tests.cpp`,
`tests/own_vector_capture_tests.cpp`, test-only comparison helpers as needed;
modify `OwnNativeEllipsePreparedAsset.hpp/.cpp`, `OwnNativeEllipseStream.hpp/.cpp`,
`OwnNativeEllipsePlayback.cpp`, `SourceGeometryProjector.cpp` only for shared pure
helper extraction, and `CMakeLists.txt`.

**Interfaces:** immutable program contains render-layer intervals and ordered
draw bindings (source IDs, optional trim, geometry/paint resource IDs/classes).
Prepared owner retains its primitive `authored` field and gains `vectorAuthored`
and `program`; exactly one authored alternative exists. Add
`OwnNativeEllipsePrepareResult prepareOwnVectorAsset(shared_ptr<const OwnVectorModel>)`
and `OwnNativeEllipsePrepareResult prepareOwnMotionAsset(const OwnJsonDocument&)`.
Add result `path/message` without weakening existing code/boolean contracts.
Profile selection is structural once; unsupported vector input cannot retry a
renderer or silently drop features. Stream/playback/Player API stays unchanged.

- [ ] Functional RED: real Duck compiles but preparation/stream emission unavailable;
  first-party strokes/trim/morph demonstrate observable missing output. Verify
  assertions fail before implementation. Keep old primitive/singleton tests.
- [ ] Lower both profiles to one program and one stream iteration. Reuse existing
  evaluator, primitive generation, pure shape materialization, trim generator,
  model application and planner. SourceGeometryProjector keeps its oracle checks.
  Build correct render layers/order, source binding, stroke metrics, separated
  transforms/opacity and per-local-dependency static classes. No fabricated
  reference owner/asset handle. Playback reads common model frame rate with old
  float-rounded mapping; primitive timing tests remain exact.
- [ ] Run actual TGS -> own -> all180 forward/reverse scenes and repeated seeks,
  viewport128->512->128, frame179/loop and owner retirement. Compare all visible
  path/paint/stroke/order/opacity/transform fields against fresh ordinary oracle
  under exact/spec1e-4 criteria with maximum errors reported. Omit no visible
  field to get green. Separate intentional source topology mapping from pixels.
- [ ] Test two streams and 1/4/16 sequential instances, hidden/reactivated layers,
  static path plus moving parent and changing trim, failed-frame atomicity,
  invalid viewport and cache/identity history. Mutation tests catch trim removal,
  inherited parent opacity, stroke/fill reordering and a changed curve.
- [ ] Test WARP captures at frames0,10,15,20,45,90,110,135,179 at128/256/512;
  investigate every nonzero same-backend diff. Reuse existing CPU capture policy.
  If reference-plan policy blocks comparison, report to controller for a written
  test-only extraction decision; do not change public fallback policy. Store raw
  own/reference images locally and inspect images, not only hash/text results.
- [ ] Full none/Telegram and explicit preview CTest with current sources, then
  independent task review/root focused check; update STATE/ledger, scoped commit
  `feat: emit own vector sticker scenes`, ordinary push. Do not stop at engine
  success: Task4 is the user's milestone.

### Task 4: Same UI, verified static candidate and canonical promotion

**Files:** host `src/app/motionlab/OwnMotionRenderer.cpp/.h`,
`src/app/motionlab/MotionLabPage.cpp` only for the truthful badge/size diagnostics,
`MotionTypes.h` and `MotionWorker.cpp` only for actual-raster diagnostics (no
scheduling/control changes); engine private `OwnNativeEllipsePlayback.hpp/.cpp`
and its existing test only for minimal immutable prepared-owner access;
`tests/motionlab/tst_own_motion_renderer.cpp` and actual corresponding own page/
worker test files discovered from `tests/motionlab/CMakeLists.txt`, focused test
support if needed; host current-release/own Motion Lab docs and engine report/
STATE/ledger. Do not edit unrelated window/docking/tray code. Existing build
instruments under ignored `out/part26l` may be adapted into `out/part26n` without
overwriting earlier evidence; host artifacts use build only.

**Interfaces:** host reader explicitly selects65536/depth32 and invokes
`prepareOwnMotionAsset(document)`. Same prepared pointer and worker/playback
types, owned premultiplied QImage output, existing metadata/viewport limits and
error handling. No new panel, public loader, worker or timer.

Ruling16: vector-only composition-aspect-fit targets within the already bounded
request; unchanged MotionCanvas centers the returned QImage. Preserve primitive
and reference sizes. Checked integer fit, explicit unrepresentable-size errors,
no retry/fallback/extra padding bitmap or second scale pass. Evaluation, texture,
readback and image dimensions agree. Distinguish request, bounded box and actual
successful raster in diagnostics; do not report stale old-asset sizes after
replacement/failure. Read viewport-decision.md for named integration pitfalls.
Cover wide/tall/odd sizes, non-square logical dimensions and tiny bounds in
first-party tests, and real Duck wide/tall1/4/16 UI. Do not claim bit-identical
widget resampling or direct rectangular-engine support from this adapter.

- [ ] Read ROADMAP, maintenance and system-tray contracts. Functional RED through
  actual host own load of explicit original TGS; verify old preparation rejects
  it. Add path input to test harness, not production baked file/asset lookup.
- [ ] Switch only preparation entry/diagnostics, retain controls. Own badge becomes
  `Experimental — own vector subset / WARP readback`; reference badge unchanged.
  GREEN actual
  Duck load/play/pause/seek/resize/visibility,1/4/16instances, errors/limits,
  stale completion and shutdown. Capture distinct actual own page frames for
  visual inspection; label Qt test-page versus canonical-native evidence.
- [ ] Run existing dynamic Qt reference/own/working-entry suites and host
  deployment/diagnostics/build/runner tests relevant to release handling. Build
  under fresh host build/cmake trees using existing dependencies only. Read
  actual warning/skip/test totals, no assumed all-green baseline.
- [ ] Root assembles whole-stage review package, independent most-capable review;
  at most one combined final fix wave and one scoped re-review. All load-bearing
  residuals are explicitly dispositioned before promotion. Run fresh engine
  none/TG/preview gates after final product fixes; no unrelated repeat stages.
- [ ] Build fresh own-static Qt6.10 /MT app+smoke candidate from frozen sources;
  full26-file notices/import/graph/link/source/PDB audit and install prohibition.
  No rlottie/Reference graph/link. Reuse tested recoverable promotion instrument
  with target/source/backup validation and fault-recovery tests. Recheck current
  canonical hash/process, save entire26-file package+PDB, promote, verify copied
  hashes/readiness/symbol match. Never terminate an unknown/user process.
- [ ] Exercise canonical application through available native tools and ordinary
  owned-process interaction under the spec's normal-app-side-effect clarification.
  Do not claim settings isolation or manually change Windows/registry settings.
  If unavailable, explicitly record that native GUI/
  DPI/tray remains unverified; Qt capture alone does not complete that check.
  Preserve verified candidate if the required acceptance cannot safely run.
- [ ] Update user-facing report with exact engine/hostSHAs, commands/results,
  source sticker identity, own/reference captures, real timings/counters and
  limitations. Scoped docs/host commits and ordinary pushes after remote check.
  Milestone complete only for actual unchanged Duck in this same UI; no full
  Telegram compatibility, speedup, zero-allocation or hardware GPU claim.

## Execution controls

Each new functional path gets RED before implementation; run focused tests while
iterating and full required suite at the block boundary, not after every edit.
One product implementer, fresh task reviewer, root-only Git. Read-only audits can
run in parallel. Keep `.superpowers/sdd/2026-09-28-own-duck-playback` and raw
evidence per user override of cleanup. Do not repeat completed tasks after
compaction. Controller rulings go in durable ledger with cost if wrong.
