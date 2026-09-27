# Part26G Own Ellipse Model Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build and privately publish the constrained own ellipse model and canonical resources without a reference engine.

**Architecture:** Runtime owns shared numeric interpretation/binding and a sealed authored-model factory; Rendering owns primitive/resource preparation. Existing static targets and Model helpers keep the dependency graph acyclic. Legacy certificate/loader/host routes remain intact.

**Tech Stack:** Existing C++20, MSVC2022, CMake/Ninja/CTest; no new dependencies.

**Spec:** docs/superpowers/specs/2026-09-27-own-ellipse-model-design.md, controller-approved efc12e9.

## Global Constraints

- Direct main, small scoped commits and ordinary push after verification/review; no force-push, rebase or history rewrite. One product writer; preserve raw/SDD evidence.
- No vendor, dependency, license/notice, fixture or golden changes; no dependency installation or Windows/settings/security/automation actions.
- No UI writes/builds/GUI or UI/out recreation; preserve accepted Avelabs712d454 and Release EXE C92F26EE8FEB4FF03F6DC7F4AFFF4B11FB48142743D1EDCCB4E213AAA81F82C2.
- No Runtime.cpp loader/identity changes, Player threading, Direct2D ownership, ANGLE/backend coverage or public fallback changes. All new interfaces are private/not installed; reference-linked install prohibition stays.
- NativeEllipseCertificate, NativeEllipseScanAudit, legacy admission/input contracts and all existing tests remain unchanged. NativeEllipseStream changes only by extracting its identical first-party path materializer, not its authorization, emission or lifetime policy.
- OwnJsonReader and the F grammar/adapters are unchanged; do not copy vendor parsing, add a mutable own-document builder, reserialize, or fall back to a reference route.
- No full Lottie, own playback/GPU, universal scalar parity, TSan, zero-allocation or measured-speedup claim. Samsung is configure/provenance-only in this stage.

## Review Focus

1. Exact admission can accept tiny values that float-model preparation cannot; reject conversion without publishing or changing F diagnostics. Task1 tests rate/size/color underflow, noncanonical decimals and subnormal success.
2. Two sources can have identical topology but distinct values; workspaces, source bytes and names must not cross-contaminate. Task1 tests source destruction, distinct fingerprints, wrong workspace and parallel preparations.
3. Default metadata/unused fields and value-row aliasing can conceal a malformed graph despite correct counts. Task1 literal all-field assertions plus mutated/reindexed copies and binder checks exercise this.
4. A positive representable size can collapse during local path arithmetic; resource failure must not erase earlier immutable results. Task2 pins static collapse, null input and retained alias ownership.
5. Shared constructors/comparators can agree on the same mistake; Task2 literal geometry/paint hashes and Task3 comparator mutation witnesses supplement semantic differential comparison.

## File map and verification environment

Task1 creates private Runtime NativeEllipseNumeric.hpp/.cpp and
OwnNativeEllipseModel.hpp/.cpp; extracts only numeric interpretation from
NativeEllipseBinding.cpp and compiles that binder unconditionally in Runtime.
Task2 creates Rendering NativeEllipsePathMaterializer.hpp/.cpp and
OwnNativeEllipsePreparedAsset.hpp/.cpp; shares existing Model hashes/summary through
AssetModelBuilder.hpp/.cpp, mechanically changes old NativeEllipseStream.cpp to
call the extracted path helper. Task3 is new tests/support only. All modify their
own scoped CMake registrations; no public include changes or new targets beyond
test executables. Tests/support/NativeEllipseAdmissionTestData.hpp is reused
unchanged; create OwnNativeEllipseModelTestData.hpp for shared independent tests.

Use installed BuildTools VsDevCmd.bat -arch=x64 -host_arch=x64 before MSVC commands.
Quote CTest regexes when passing through cmd.exe; retain environment/quoting failures
separately from functional RED. Raw out/part26g/task-N. Worker task reports remain
local under .superpowers/sdd/2026-09-27-own-ellipse-model/, never force-add them.
Root owns docs, guarded ordinary pushes and final broad gates; workers no push,
subagents, UI or concurrent product writers. First baseline is c00f686.

### Task 1: Shared numeric contract and sealed own authored model

**Files:** NativeEllipseNumeric.hpp/.cpp, OwnNativeEllipseModel.hpp/.cpp under
src/runtime; modify NativeEllipseBinding.cpp and CMakeLists.txt; new
tests/native_ellipse_numeric_tests.cpp, tests/own_native_ellipse_model_tests.cpp,
tests/support/OwnNativeEllipseModelTestData.hpp.

**Interfaces:** namespace avemotion::runtime::detail:

```cpp
struct NativeEllipseNumericValues final {
    float frameRate = 0;
    model::MotionVec2Value translation, size, start, end, outgoing, incoming;
    model::MotionColorValue color;
    bool animated = false;
    std::uint32_t firstFrame = 0, lastFrame = 0;
};
std::optional<NativeEllipseNumericValues> interpretNativeEllipseInput(
    const NativeEllipseInput&);
enum class OwnNativeEllipseModelCode {
    Prepared, AdmissionRejected, UnsupportedNumericConversion, ModelConstructionFailed
};
// OwnNativeEllipseModel: private constructor; friend factory below only.
// Public CONST members: std::string exactJson;
// shared_ptr<const NativeEllipseInput> input;
// shared_ptr<const model::MotionAssetModel> model;
// NativeEllipseNumericValues values; NativeEllipseModelBinding binding.
struct OwnNativeEllipseModelResult final {
    OwnNativeEllipseModelCode code = OwnNativeEllipseModelCode::ModelConstructionFailed;
    NativeEllipseAdmission admission;
    std::shared_ptr<const OwnNativeEllipseModel> prepared;
    explicit operator bool() const noexcept; // Prepared && prepared != nullptr
};
OwnNativeEllipseModelResult buildOwnNativeEllipseModel(
    const formats::detail::OwnJsonDocument&);
```

Forward-declare OwnJsonDocument in model header. Factory constructor takes owned
values by value/move; no public constructor accepting rows, const_cast mutation,
source-hash argument or Runtime call. Document-only API, not a byte loader.

- [x] **Step 1: Numeric stub functional RED.** Add numeric source to Runtime and
  all-variant avemotion_native_ellipse_numeric_tests / CTest
  avemotion.runtime.native_ellipse_numeric, private runtime/formats/testsupport
  includes, Runtime link. Tests use expectedEllipseBaseline() literal descriptor.
  Stub returns nullopt, successful build followed by assertion failure
  `baseline numeric values interpreted`. Pin full expected60,translation256/256,
  size120/120,start-76/0,end76/0,controls0.333/0 and0.667/1,color0.08/0.72/0.95/1,
  animated=true,frames0/60. Expected float casts from literals, not normalizer.

- [x] **Step 2: Extract numeric implementation and preserve binder.** Move only
  canonical/convert/interpret code into numeric cpp; one compiled implementation,
  same check ordering and failure behavior, optional returned only on full success.
  Replace binder ExpectedValues/interpret call with this API and move its source
  out of Telegram guard into unconditional Runtime sources. Keep old binder header
  and all topology/property/value/track checks unchanged. Preserve static unused
  fields' original value initialization (including MotionColorValue alpha default).
  Extend tests for fr1e-9999,size/color1e-9999 rejecting, private malformed coefficient/
  exponent/sign/negative-zero/overflow rejecting; size1e-45 == float denorm_min,
  fr59.94 == float(59.94), signedzero normalized, static position boundary32768.
  Run numeric GREEN plus unchanged Telegram binding/certificate tests.

- [x] **Step 3: Own-model stub functional RED.** Add own source and all-variant
  avemotion_own_native_ellipse_model_tests / avemotion.runtime.own_native_ellipse_model,
  Runtime+Threads links, private runtime/formats/testsupport, fixture directory.
  Stub returns ModelConstructionFailed; compile and observe `own authored model
  prepared` failure for readOwnJson(ellipseFixture()) before constructing rows.

- [x] **Step 4: Construct model and sealed publication.** Factory F-admits document,
  interprets values, builds exact spec table conventions and statistics. sourceAssetHash
  is core::fnv1a64(as_bytes(span(exactJson))); parsed fingerprint uses two length-
  prefixed appendString calls with exact domain AveMotion.OwnEllipse.Authored.v1
  and exactJson. Invalid AssetHandle, revision1; no render rows or derived render
  fingerprints. Bind and validate/prepare temporary PropertyEvaluator workspace;
  failure publishes nothing. Freeze with no surviving mutable alias, private-own
  wrapper holds exact bytes/input/values/binding/model. Do not scan/evaluate frames
  during construction. F owns admission diagnostics; numeric failure keeps Accepted
  admission but separate model code. Allocation exceptions retain existing semantics.

- [x] **Step 5: Independent all-field graph/evaluation tests.** Shared test helper
  assertOwnAuthoredModel(prepared, literalInput, literalNumericValues) asserts every
  field listed in spec: metadata, all five complete node records/defaults/names/hashes,
  all property/edge/typed-value references, all track/segment fields, counts, empty
  unused tables, absent render rows and invalid handle. No product binder or model
  constructor generates expected records. Independently length-prefix/hash expected
  labels/source/domain bytes in test code, with raw-hash versus appendString witness.

  Cases: baseline; static boundary position; linear controls(0,0)/(1,1), cubic baseline;
  active10..20 independent of track0..60; indINT32_MAX; translation-32768/32768;
  size16384/0.5;color0/1/0.4/1;fr59.94/equivalent6000e-2; all7 names absent/empty,
  UTF8 snowman and literal label `a.b/[x]~`; numeric subnormal1e-45 size as authored
  only; fr/size underflow separately; default empty document InvalidJson `/`, missing
  fields retains F path, every failure prepared==nullptr and earlier result intact.
  Exact bytes must remain unchanged including whitespace and distinguish absent nm.

  Evaluator tests use literal linear sample at frame30:position0/0, endpoints-76/0
  and76/0; static position unchanged. Pin local/world layer/group matrices and
  opacity. Copy returned values before next evaluate (views are borrowed). Order
  [60,0,30,30,1,59,0] matches per-frame literal linear calculation. Compare model
  fields unchanged after evaluator use. Existing binder validates own model; mutated
  copy wrong owner/aliased scalar/extra row/reordered edge rejects. A semantically
  reindexed copy still binds (test-only remap; factory itself remains dense).

  Destroy document/source but retain wrapper and all-field assertions. Prepare two
  models with same shape but different translation/name; prepared workspaceA passed
  to evaluatorB yields WorkspaceNotPrepared, separately prepared B succeeds.
  Two threads×64 each own document/model/workspace, names A/B and translations11/22,
  33/44; compare literal values and retained serial results. No shared mutable
  workspace or assertion counter; this is functional isolation, not TSan.

- [x] **Step 6: Verify, report and commit.** None numeric/model focused then full
  none once. Telegram numeric/model plus unchanged binding/certificate/stream/lifecycle
  and all F tests, quoted regex. Full Telegram/preview belong to root after Task3.
  Inspect binder diff only extraction; source ownership/private headers; git diff
  --check. Record first functional failures, exact commands/results and limits.
  Commit scoped files `feat: construct own authored ellipse models`; no push.

### Task 2: Canonical resources and distinct sealed prepared asset

**Files:** create src/render/NativeEllipsePathMaterializer.hpp/.cpp,
OwnNativeEllipsePreparedAsset.hpp/.cpp; modify NativeEllipseStream.cpp only helper
extraction, src/model/AssetModelBuilder.hpp/.cpp only shared private helper access,
CMakeLists.txt; create tests/own_native_ellipse_resources_tests.cpp, extend new
OwnNativeEllipseModelTestData.hpp only for independent resource expectations.

**Interfaces:** Rendering private namespace avemotion::render::detail:

```cpp
bool materializeNativeEllipsePath(const PrimitivePath&,
    const runtime::AffineTransform*, runtime::EvaluatedPath&); // fresh empty path
enum class OwnNativeEllipsePrepareCode {
    Ready, InvalidModel, ResourceConstructionFailed, ModelConstructionFailed
};
// OwnNativeEllipsePreparedAsset: private constructor/friend factory; public CONST
// shared_ptr<const runtime::detail::OwnNativeEllipseModel> authored;
// shared_ptr<const model::MotionAssetModel> model.
struct OwnNativeEllipsePrepareResult final {
    OwnNativeEllipsePrepareCode code = OwnNativeEllipsePrepareCode::InvalidModel;
    std::shared_ptr<const OwnNativeEllipsePreparedAsset> prepared;
    explicit operator bool() const noexcept; // Ready && prepared != nullptr
};
OwnNativeEllipsePrepareResult prepareOwnNativeEllipseAsset(
    std::shared_ptr<const runtime::detail::OwnNativeEllipseModel> authored);
```

Model private helpers in avemotion::model::detail, same AssetModelBuilder.cpp:
uint64_t hashCanonicalGeometry(runtime::FillRule,const runtime::EvaluatedPath&) noexcept;
uint64_t hashCanonicalPaint(const runtime::EvaluatedStroke&,const runtime::EvaluatedPaint&) noexcept;
void refreshAssetModelDerivedData(MotionAssetModel&) noexcept.
Wrappers may call existing anonymous functions; never copy their algorithms or
change old scan call sites. Path helper extracts the old function including hash
and control bounds unchanged, no transform-order/fingerprint changes. Fresh-empty
destination is the private call precondition and all call sites must satisfy it.

- [x] **Step 1: Prepared-resource stub functional RED.** Add Rendering sources
  unconditionally; Rendering private include runtime/formats/model for factory,
  no Runtime->Rendering edge. Test target avemotion_own_native_ellipse_resources_tests /
  avemotion.render.own_native_ellipse_resources links Rendering+Threads, fixture/TGS
  directories and private headers. Build successful own authored model then stub
  preparation; observe `own resources prepared` failing before resource construction.

- [x] **Step 2: Extract helpers and construct resources.** Share Model hashes/
  summary and stream path materializer mechanically. Build the exact two-layer/
  one-node/geometry/paint/default-loop-clip convention from spec; model initially
  private copy of authored, revision remains1, invalid AssetHandle. Literal root
  label __, shape label effective authored layer name, no selector semantics.
  Node dependencyBits is Transform alone for static position, Transform|Geometry
  for animated position, matching actual InstanceEvaluated local geometry.
  For static local path, generateEllipsePath and materialize with null transform;
  error or empty/collapsed output aborts own publication. Existing generator returns
  valid empty primitive for a collapsed rectangle and old materializer accepts it;
  put the nonempty check in the own factory, never alter the shared old helper's
  behavior. Animated geometry no canonical data/hash0. Paint bytes
  truncate255*component, alpha255; all unused/default stroke/gradient/image fields
  initialized. Source keys1; shared hashes; exact declared stats then refresh derived
  summary. Binder verifies final model, then freeze and publish with retained owner.
  Never call reference preparation, NativeEllipseCertificate or a full timeline scan.

- [x] **Step 3: Literal resources and fail-atomic lifetime tests.** Unit static
  position0/0,size2/2: verbs MoveTo,4 CubicTo,Close,13 points
  (0,-1),(k,-1),(1,-k),(1,0),(1,k),(k,1),(0,1),(-k,1),(-1,k),(-1,0),
  (-1,-k),(-k,-1),(0,-1), k=0.5522847498F, bounds[-1,-1,1,1]. Independent
  literal-byte FNV computation (root JS, no product helper) gives path
  0x501de312ce7d322a, Winding geometry0x6862cc6516f16586. Pin these literals.
  RGB0.5/0.1/0.999/1 ->127/25/254/255, paint hash0x8485243b1edad747; also0/1
  channel endpoints. No production helper builds expected geometry/hash/paint.
  Explicitly pin the different static/animated node dependency bits.
  Assert all render row/default fields, exact summary counts and independently
  encoded topology/resource/full hashes, not only counts/hash equality. Authored
  source/property rows and parsed fingerprint remain identical to Task1 owner.

  Static source position[-32768,32768] with size120 succeeds, translated layer
  does not alter local geometry. Animated baseline has no canonical geometry and
  paint remains static. Active10..20 still default clip0..61. Names absent/empty
  and UTF8/literal delimiters follow spec byte labels. Existing telegram_sticker_basic.tgs via
  formats::decodeTgsFile -> own reader -> own model -> preparation succeeds in none.

  Static position[-32768,32768],size[1e-45,120] passes authored model but primitive
  rectangle collapses: expect ResourceConstructionFailed/null prepared; prove
  that exact cause before claiming this witness. Null owner InvalidModel/null.
  Materializer direct invalid PrimitivePath and nonfinite transform return false
  (fresh output), no own publication. Previously prepared objects are unchanged.

  Construct an independent EvaluatedScene with matching source hash, draw/node/
  geometry/paint0 and local geometry/paint available; applyAssetModel must alias
  exact addresses of staticValue fields with model owner. For animated geometry,
  origin InstanceEvaluated/no canonical; paint still aliases. Drop caller source,
  authored wrapper and prepared owner while retaining scene aliases; values stay
  readable. Wrong source hash fails without pretending application succeeded.
  Two independently prepared assets differ in paint/translation/source bytes;
  no cross-sharing of mutable models, and actual immutable static pointers remain
  stable across repeated application. No TSan or zero-allocation claim.

- [x] **Step 4: GREEN and old stream regression.** Full none once; Telegram new
  resources/numeric/model/F tests and unchanged old stream/lifecycle/binding/certificate
  plus model/canonical/plan tests. Preserve all old golden files. Inspect helper
  diff identical arithmetic, CMake dependency direction/private export. Scoped commit
  `feat: prepare own ellipse canonical resources`; task report, no push.

### Task 3: Independent semantic comparison and cross-model lifetime evidence

**Files:** create tests/own_native_ellipse_model_differential_tests.cpp; extend only
new OwnNativeEllipseModelTestData.hpp as needed; CMake Telegram-only registration.
No production changes without a concrete reported defect/retained failing test.

**Interfaces:** Task1 buildOwnNativeEllipseModel and Task2 prepareOwnNativeEllipseAsset;
oracle prepareNativeEllipseCertificate(json) from unchanged old path; each side
owns distinct input/model/reference Asset and workspace. No fixture mutation or
borrowing candidate prepared data to build oracle. CTest
avemotion.runtime.own_native_ellipse_model_differential, target
avemotion_own_native_ellipse_model_differential_tests, links Rendering, private
render/runtime/formats/model/testsupport, existing fixture directory.

- [x] **Step 1: Functional comparator RED.** Implement test-only semantic snapshot
  DTO and initially false/placeholder comparator; equal independent literal
  snapshots must fail a successful-build `equal semantic snapshots compare` assertion.
  Complete comparator before oracle matrix. Compare semantic roles/edges/defaults,
  resolved property types/flags/values and track/segment/timing, evaluated local/world
  transforms/opacities, geometry/paint contents/resource classes and render-node
  dependency semantics (static Transform, animated Transform|Geometry). Exclude raw
  IDs, revision/source/parsed/full hash identities, Asset handles and arbitrary-name render
  labels by explicit schema, not a field-skipping whitelist at first mismatch.
  Comparator self-tests change one value, semantic role, node enabled bit, keyframe
  control, path point, paint byte and resource class; each must compare different.
  Report what this matrix proves; no pixel or general grammar claim.

- [x] **Step 2: Common-domain matrix with literal anchors.** Cases baseline,
  static boundary, static origin unit, linear controls, active10..20, translated
  layer, fractional59.94/equivalentrate, changed size/color, all names absent/empty,
  UTF8 snowman. Assert each builder/old certificate succeeded before comparing.
  Resolve source/property roles by each side's binding rather than dense indices.
  Compare semantic snapshots and evaluator samples at[-1,0,0.5,10,30,60,61,30,1,0]
  with separate per-side workspaces; copy borrowed results before next sample.
  Independently pin baseline endpoints, unit static path/paint and linear midpoint.
  Check static resources both sides; animated geometry must remain evaluated.
  The independent literals in Tasks1/2 remain the own path's non-oracle correctness
  proof. Do not require raw model-row/hash equality with reference extraction.

- [x] **Step 3: Deliberate policy and isolation rows.** Raw ip=0e999999999999999999999999
  accepted/prepared own baseline but old AdmissionRejected; escaped low-surrogate
  is reader InvalidJson (no own model call), legacy input may accept but record old
  certificate actual bounded outcome, not assumed. Tinyfr1e-9999 own model returns
  UnsupportedNumericConversion, no prepared; explicit no fallback. These are literal
  policy cases separate from successful semantic comparisons, not skipped failures.
  Retain own prepared model/source/resources, destroy all oracle/runtime objects;
  repeat own evaluation and model application and assert values. Two own prepared
  sources must keep distinct bytes/model pointers without requiring unique FNV as
  security. No reference participation in construction or none tests.

- [x] **Step 4: Focused GREEN, report, scoped commit.** Telegram comparator and all
  new G tests plus C/D stream/binding/certificate regression; no duplicated full
  platform runs. Record executed cases/samples/assertions from real counters, first
  failures and full comparison exclusions. git diff --check, commit test/CMake only
  `test: compare own ellipse models with reference semantics`; no push.

## Root final gates, preflight and handoff

- [x] Task1 review/fixes/STATE/ledger/ordinary guarded main push; then Task2, then Task3. SDD workspace reports retained, no duplicate live writer.
- [ ] Final full MSVC configure/build/CTest none, Telegram, Win32-preview; explicit capture/header/contract/smoke/preflight/preview-selftest; no skipped required tests.
- [ ] Samsung configure/listing only; both vendor trees/corpus/TGS16; no license or golden updates.
- [ ] Actual include/link/source ownership and no Runtime->Rendering cycle; private headers uninstalled, unchanged production route/old certificate; UI hash/out absence.
- [ ] Whole-stage review fromc00f686, at most one combined final fix wave/scoped review; fresh final gates for changed product/instruments. Record all decisions/costs and limitations.
- [ ] Separate next own stream/identity/frame/pixel design, then isolated static UI acceptance. No new automation.

Preflight self-review maps numeric policy/authored rows to Task1, render resources/
sealed ownership/helper extraction to Task2, independent semantic matrix to Task3,
platform/closure to root. Task2 consumes the exact sealed Task1 wrapper, not a
mutable model or fake Runtime Asset. Test helper is new, old tests unchanged. Each
Review Focus item has explicit owning tests. All global constraints are copied
verbatim from spec. Spec+plan approved by controller under delegation; select SDD
for three independently rejectable boundaries with one writer and fresh reviews.
