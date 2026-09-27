# Own Primitive Groups Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Render the unchanged seven-shape primitive_geometry JSON/TGS through the existing own engine and canonical Avelabs UI.

**Architecture:** Extend one private own model/preparation/stream route to ordered bounded primitives. Share parsing and numeric helpers, keep the separate strict reference certificate contracts, and retain Player/worker/timer/output ownership. Separate behavioral oracle tests link pinned Telegram; the working app remains no-reference.

**Tech Stack:** C++20, MSVC 2022, CMake/Ninja, Qt 6.10, Direct2D/WARP; static host /MT and existing dynamic QtTest /MD.

**Spec:** docs/superpowers/specs/2026-09-27-own-primitives-profile-design.md (0cbe264).

## Global Constraints

- One2D shape layer with1..16 direct groups, subject also to current4096 value-node limit. Each group is exactly `[el-or-rc, fl, tr]`, with optional inert names.
- Root dimensions1..8192, totalframes2..10000, frame rate(0,240], position and layer translation±32768, sizes(0,16384], radius[0,16384].
- Both primitive directions `d=1/3`; keys exactly0 androotOp-1, terminal start equals previous end by exact decimal comparison. Reject extra/hold/spatial/per-axis-different interpolation.
- New profile accepts absent ao or explicit0. Legacy admission/input/numeric/binder/certificate/scan stay strict; no shadow legacy payload in production owner.
- No fixture-name/hash special cases, unsupported-prefix output, fallback, public API expansion, new player/worker/timer, dependency/Windows change, vendor/golden/corpus modification or licensing decision.
- Pinned Telegram oracle67f103bc8b625f2a4a9e94f1d8c7bd84c5a08d1d is test-only; production none must not link/call Reference/rlottie.
- Preserve old single-ellipse literal table IDs, fingerprints/resource hashes and strong tests. Preserve raw/SDD and every failed attempt.
- Root-only Git on existing main, scoped commits and ordinary pushes after review/remote checks. One product writer; no worker Git/subagents. Keep installed L EXE/PDB until final verified candidate and backup.

## Review Focus

- Unsupported seventh group or malformed later numeric value: reject whole input and preserve previous usable UI session (Tasks1/3).
- Array easing with differing axes or unrepresentable nonzero decimal: reject without partial owner, distinct admission/conversion result (Tasks1/2).
- Reversed draw order hidden by separated shapes: explicit source-role order plus overlapping differently colored primitives must match oracle (Task2).
- Equal values accidentally alias distinct group IDs/resources: exhaustive graph/property/value ownership and mutation tests (Task2).
- Multiple instances/resize/replacement retain stale pointers or cached paths: shared immutable owner, independent streams and owned images through retirement (Tasks2/3).

## File and execution map

Engine root is C:/Users/USER/Desktop/AveMotion-CorpusLab-Part24 (E); host root is
D:/rvc/c++/DragonianVoice/Avelabs-UI (H). Root owns this plan, spec, durable ledger,
STATE and reports; do not add ceremonial product edits to satisfy file lists.
Task1 provides independently testable admission/numeric payloads without factory
switch. Task2 migrates the tightly coupled owner/binder/preparation/stream atomically.
Task3 verifies the same host route, Task4 freezes/reviews/gates/promotes. No L repeat.
Use VsDevCmd.bat -arch=x64 -host_arch=x64 via an ignored .cmd launcher, then installed
CMake/CTest. E raw out/part26m; H raw build/checks/part26m, never H/out.

### Task 1: Bounded own primitive admission and numeric payload

Execution outcome: Steps1..5 complete in ab8800a + fix520297f, independently
reviewed and accepted. Full none43/TG93 before localized fix; final focused
none9/TG9/root8. Remaining Minor items and raw failures are retained in ledger.
Checkboxes below preserve the original prospective steps; this outcome and SDD
completion line are the execution checkpoint.

**Files (E):**
- Create src/runtime/OwnPrimitiveInput.hpp, OwnPrimitiveNumeric.hpp/.cpp.
- Modify src/runtime/NativeEllipseAdmissionCore.hpp/.cpp and OwnNativeEllipseAdmission.hpp/.cpp for shared value-table auditing/projection and new entry point.
- Modify NativeEllipseNumeric.cpp; create NativeEllipseNumericHelpers.hpp for shared private decimal/Vec2 conversion only.
- Create tests/own_primitive_admission_tests.cpp and tests/support/OwnPrimitiveTestData.hpp; register source/test in CMakeLists.txt.
- Existing legacy admission/numeric tests stay on their old entry points. No model/render/host switch in this task.

**Interfaces (namespace avemotion::runtime::detail):**
- OwnPrimitiveKind {Ellipse, Rectangle}; OwnPrimitiveGroupInput with kind, model::SourcePathDirection direction, NativeEllipsePosition position/size, optional<OwnPrimitiveScalar> roundness, fillColor[4], optional groupName/primitiveName/fillName/transformName.
- OwnPrimitiveStaticScalar {NativeEllipseDecimal value}; OwnPrimitiveAnimatedScalar {uint32_t firstFrame,lastFrame; NativeEllipseDecimal start,end; NativeEllipseVec2 incoming,outgoing}; OwnPrimitiveScalar is their variant.
- OwnPrimitiveInput has common fields named as NativeEllipseInput: width,height,endFrame,frameRate,layerId,layerInFrame,layerOutFrame,layerTranslation,version,name,layerName; vector<OwnPrimitiveGroupInput> groups. Exact payload structs support equality.
- OwnPrimitiveInputResult {NativeEllipseAdmission admission; shared_ptr<const OwnPrimitiveInput> input; explicit operator bool() const noexcept}; decodeOwnPrimitiveInput(const formats::detail::OwnJsonDocument&) -> OwnPrimitiveInputResult.
- evaluateOwnPrimitiveValues(span<const NativeEllipseValue>, shared_ptr<const OwnPrimitiveInput>* output=nullptr) -> NativeEllipseAdmission is a private shared-table seam, not an installed/public API.
- OwnPrimitiveVectorValues {MotionVec2Value start,end,incoming,outgoing; bool animated; uint32_t firstFrame,lastFrame}; OwnPrimitiveScalarValues equivalent scalar start/end with Vec2 incoming/outgoing.
- OwnPrimitiveGroupValues {OwnPrimitiveVectorValues position,size; optional<OwnPrimitiveScalarValues> roundness; MotionColorValue color}; OwnPrimitiveNumericValues {float frameRate; MotionVec2Value translation; vector<OwnPrimitiveGroupValues> groups}.
- interpretOwnPrimitiveInput(const OwnPrimitiveInput&) -> optional<OwnPrimitiveNumericValues>. Types live in the new two headers; model values use existing avemotion::model types. Kind/direction remain authoritative in input, not duplicated numeric records.

- [ ] Step 1: Add typed declarations and rejecting entry/converter scaffolds plus real fixture tests. Functional RED: decodeOwnPrimitiveInput on unchanged JSON must return an accepted seven-group owner; current scaffold rejects. Record this assertion failure, not a missing-symbol/compiler error, before admission implementation.
- [ ] Step 2: Implement shared projection and profile-aware Auditor in NativeEllipseAdmissionCore, reusing object/type/exact-decimal/pointer logic. Legacy method follows existing rules; new method validates all groups before materializing. Preserve 4096 nodes, strict unknown fields and exact equality. New position/size use scalar or dimension-matched equal-axis easing; scalar radius uses numeric static k and length-one segment endpoints. No duplicate entire auditor/parser.
- [ ] Step 3: Assert JSON and manifest-pinned TGS decode to equal exact payloads, 7 kinds/directions in fixture order, absent/empty names, all five segments 0..60 and literal endpoint/easing values. Cover 1/16 acceptance and 0/17 refusal, later unknown/malformed group, wrong it order/count, ao0/missing/1, direction1/3/2, all shape/property bounds, non-neutral transforms and wrong opacity/fill. Keep old one-ellipse decoder acceptance/refusal tests unchanged.
- [ ] Step 4: Functional numeric RED then implement interpretOwnPrimitiveInput by sharing existing canonical finite/nonzero-underflow conversion helpers. Assert expected fixture values and static/animated flags, malformed typed decimals and bounds, accepted tiny nonzero decimal becoming conversion failure, lexical zero accepted, unequal Vec2 easing components rejected at admission, terminal endpoint mismatch rejected exactly. Failed outputs have no input/values; caller sentinel owner unchanged.
- [ ] Step 5: Focused build/CTest -R 'own_primitive_admission|native_ellipse_admission|own_native_ellipse_model' under windows-msvc-direct2d, then full configure/build/CTest for windows-msvc-direct2d and windows-msvc-telegram-debug. CTest --output-on-failure --no-tests=error -j 1; zero new failures/skips. Record all warnings/failures, self-review and report. Root scoped checkpoint, independent task review, ledger/STATE then push.

### Task 2: One canonical multi-group model, resources and stream

**Files (E):**
- Create src/runtime/OwnPrimitiveBinding.hpp/.cpp; modify OwnNativeEllipseModel.hpp/.cpp.
- Modify src/render/OwnNativeEllipsePreparedAsset.cpp, OwnNativeEllipseStream.cpp and NativeEllipseEvaluationHelpers.hpp (finite scalar resolver if needed).
- Adapt tests/support/OwnNativeEllipseModelTestData.hpp and checked one-group call sites in existing model/resources/stream/playback differential tests. Keep OwnNativeEllipseStreamComparison.hpp strong and one-item-only.
- Create tests/own_primitive_model_tests.cpp, own_primitive_stream_tests.cpp, own_primitive_differential_tests.cpp, own_primitive_capture_tests.cpp, support/OwnPrimitiveSceneComparison.hpp; extend OwnPrimitiveTestData.hpp; register in CMakeLists.txt. Existing primitive generator/evaluator/planner/backend are consumed, not redesigned.

**Interfaces:**
- Consumes Task1 input/numeric types and functions. OwnPrimitiveGroupBinding contains SourceNodeId group,primitive,fill; PropertyId groupTransform,groupOpacity,position,size,color,fillOpacity; optional<PropertyId> roundness.
- OwnPrimitiveBinding contains SourceNodeId root,layer; PropertyId layerTransform,layerOpacity; vector<OwnPrimitiveGroupBinding> groups. OwnPrimitiveBindingResult contains NativeEllipseBindingCode code and optional<OwnPrimitiveBinding> binding, bool conversion; bindOwnPrimitiveModel(const OwnPrimitiveInput&, const model::MotionAssetModel&) returns it. Reuse result code taxonomy, not legacy binder implementation's singleton assumptions.
- Existing OwnNativeEllipseModel factory/signatures/codes remain; input becomes shared_ptr<const OwnPrimitiveInput>, values OwnPrimitiveNumericValues, binding OwnPrimitiveBinding. PreparedAsset/Stream/Playback/Player APIs stay unchanged.
- Test-only checked single-group adapters return old descriptor/numeric/binding views by value solely for independently authored old expectations. No adapter stored in production.

- [ ] Step 1: Functional RED through current buildOwnNativeEllipseModel unchanged fixture expects Prepared/seven shapes but gets AdmissionRejected. Add whole-fixture tables/binding assertions and failure atomicity before switching factory. Compile-only migration errors are not RED evidence.
- [ ] Step 2: Build canonical source/property tables by group loop and bind exhaustively. Whole fixture:23 source nodes,22 edges,48 properties,43 static,5 animated/tracks/segments. Match exact kinds/directions/names/parents/child/property ranges/value types/endpoints. Reject duplicates, unconsumed/aliased rows, later owner/value mutation, extra/orphan/incorrect edge, wrong track endpoint. Keep all legacy reindexed binder tests and old literal IDs/fingerprints, source hash and lexical/underflow result codes via test-only adapters.
- [ ] Step 3: Functional resource RED then generalize preparation loop. Whole fixture has7 unique geometry/paint/draw slots,5 asset-static+2 instance geometry,7 static paints,2 render layers. Use existing rectangle/ellipse generator and stable old one-item hashes; no dedup alias. Validate own binding before/after preparation. Assert later corruption rejects and previously prepared owner remains usable.
- [ ] Step 4: Functional stream RED then emit N bound primitives after one workspace evaluation. Resolve Vec2/scalar values, original direction and transforms; sum stats/control bounds, preserve identity/fingerprints/repeated-frame semantics and active intervals. Test exact ordering, 1/4/16 independent instances, forward/reverse/repeated seek, changed viewport, inactive/reactivated frames and retained scene/plan after source handles retire. No new playback type.
- [ ] Step 5: In separate Telegram test binary compare full semantic fields/local+transformed paths/scene/plan for every integer frame0..60, forward and reverse, with explicit source-role/identity normalization. New multi-item comparator separately tested with path/color/order/source-role mutations. Add overlapping differently colored groups to expose order errors and compare ordinary reference, never reused naive renderTree. Preserve unchanged singleton differential comparator.
- [ ] Step 6: Capture own and ordinary-reference WARP exact bytes for frames0,15,17,30,45,60 using five existing capture profiles. CPU uses unchanged PixelComparisonPolicy: IoU .78, alpha-relative .12, mean all/active12/30, large-difference fraction .18, bounds delta4. Reuse existing lifetime/cache/resize framework, assert actual resource reuse/retirement counters; no performance/zero-allocation claim. Preserve PNG/raw failures, never change goldens/policy.
- [ ] Step 7: Focused new tests then full none (windows-msvc-direct2d), Telegram and explicit windows-msvc-win32-preview CTest; targeted Samsung '(native_ellipse|own_primitive)' (exclude only genuinely unavailable targets, not failures). Known unrelated Polystar failures remain documented. Self-review/report including every compile/setup/test failure, root checkpoint, independent task review then ledger/STATE/push.

### Task 3: Whole fixture in the same Avelabs UI

**Files (H):** src/app/motionlab/MotionLabPage.cpp and any current badge copy in MotionWorker.cpp/OwnMotionRenderer.cpp; tests/motionlab/CMakeLists.txt, tst_own_motion_renderer.cpp, tst_motion_worker.cpp, tst_motion_page.cpp, own_engine_smoke.cpp; cmake/AveMotionLab.cmake only for the static smoke fixture path; docs/build/own-motion-lab.md. Wire separate primitive JSON/TGS definitions, retaining basic fixture definitions. Only change production labels/diagnostics necessary for the expanded existing route, not scheduling/control/render ownership.

**Interfaces:** consumes unchanged prepareOwnMotionAsset/OwnMotionRenderer, Player registration, MotionWorker/Controller signals and QImage. Badge becomes `Experimental — own primitives / WARP readback`; reference badge unchanged. Existing working-release mode, build flags and package format unchanged.

- [ ] Step 1: Read host ROADMAP/maintenance/current tray and report out/part26m-design/host-inventory.md. Add functional tests using unchanged fixture JSON/TGS through actual own renderer/worker/page; assert seven visible differently colored primitives with independent pixel regions and moving final two across seeks, error-free load and truthful badge. Run before host edits; behavior already enabled by Task2 may pass, so record that as cross-repo integration coverage, not fabricated host RED. Badge expectation supplies functional failing UI test before text change.
- [ ] Step 2: Update truthful label only; adjust any old whole-fixture unsupported expectation to a still-unsupported mutation in the seventh group (e.g. stroke). Old repeater and other unsupported tests remain. Assert rejection preserves active session/generation/images; same controls play/pause/seek/1/4/16/visibility/resize/destruction, owned image survives target reuse. Existing ellipse and reference mode tests still run. Extend static smoke to actually sample whole fixture plus old basic.
- [ ] Step 3: Capture actual own test-page output at frame0 and30 with seven shapes, inspect PNGs; keep files under build/checks/part26m. No native canonical GUI or settings modification, no manual/DPI acceptance claim.
- [ ] Step 4: Serial full standalone reference/own/working-entry Qt builds/CTest using existing dynamic Qt6.10 /MD route, --config Release --parallel4, --output-on-failure --no-tests=error -j1. Run existing Python deployment/build suites and actual build-boundary test in new output root. Report totals/warnings/limitations, self-review, root checkpoint + independent task review then ledger/STATE/push.

### Task 4: Frozen final gates, static update and handoff

**Files:** root-owned docs/PART26M_OWN_PRIMITIVES_REPORT.md, STATE, ledgers; H current-release/output-layout/result report. Root scratch gate/audit/promotion instruments under E/out/part26m and H/build/checks/part26m. No new product scope.

**Interfaces:** consumes accepted Tasks1..3, L package verifier/build/promotion contract and exact frozen product inputs. Produces verified own-static canonical app with retained prior L pair; docs HEAD is not build identity.

- [ ] Step 1: Independent whole-stage review with both repo diffs/spec/plan/ledgers. At most one combined final fix and one scoped re-review; resolve/explicitly surface residuals. No controller product fixes.
- [ ] Step 2: Freeze clean engine/host SHAs; fresh sequential full MSVC none/Telegram/explicit-preview, applicable Samsung targeted suite, both vendor/corpus/TGS integrity, full host three-mode Qt/Python/boundary gates. Preserve raw hashes/commands/times, exact skips/warnings and actual graph/link/include closure; no claim clean rebuild if incremental.
- [ ] Step 3: Fresh static app+own smoke using existing dependencies in H/build/cmake/part26m-own-release-001, evidence H/build/checks/part26m-promotion-001. Configure canonical prefix but stage candidate before installation. Check26 exact package files/notices/current engine identity/local-development-only metadata, actual /MT compiler commands, no Reference/rlottie graph/link/library input, no Qt/CRT DLL imports, matching EXE/PDB via symchk, smoke and readiness.
- [ ] Step 4: Retain complete hash-verified current L26-file package/PDB backup, not old22 profile. L expected EXE93B10E5C93A3182BC59A4333F5444E95F4D4CA32D506AA05E26D1F740C5C1645 and PDB014ACD8CE9B3AFAB44A80F1ECEEAAC766D606EC958DC08DC64A392767173DFFF. Recheck canonical process; never kill user process. Recovery evidence must not prevent restoration, validate rollback; preserve original L instruments and use new paths/guards. Promote only after checks, recheck copied hashes/readiness/symbol match.
- [ ] Step 5: Update report with actual installed pair/build SHAs, test commands/counts, captures/counters, rollback location and limitations. No speedup/zero-allocation/hardware/full-Lottie/real-sticker claim. Root commits scoped docs and pushes only unchanged expected remotes. Keep raw/SDD, report Rulings and costs; no new automation or extra architecture stage merely to fill time.

## Self-review and delegated approval

Spec coverage: Task1 admission/numeric; Task2 typed transition/legacy preservation,
tables/resources/parity/order/capture; Task3 UI/replacement/owned-image; Task4 final
provenance/package. Each Review Focus risk has named tests. Shared types/functions
above use existing exact-decimal and model types; old factory names deliberately
stay private. Task1 does not switch the factory, Task2 is the atomic coupled route
change. Tests derive literal fixtures or independent reference results, not their
expected values from own builders. Bounds and zero-failure requirements are not
weakened by targeted Samsung selection. Plan has four reviewable deliverables,
not one task per field. Controller approves under explicit user delegation and
preserves SDD execution; no routine approval wait.
