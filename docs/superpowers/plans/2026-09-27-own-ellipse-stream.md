# Part26H Own Ellipse Stream Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Emit owned own-model ellipse scenes/plans without a reference production dependency and prove bounded scene/plan/pixel equivalence.

**Architecture:** New Rendering-private stream over the sealed G prepared owner, using existing evaluator/primitive/materializer/model helpers. Factory-allocated identities with own-only planner/backend domains. Independent literal, ordinary reference and WARP tests; old production routes unchanged.

**Tech Stack:** Existing C++20, MSVC2022, CMake/Ninja/CTest, test-only Telegram and D3D11 WARP/Direct2D.

**Spec:** docs/superpowers/specs/2026-09-27-own-ellipse-stream-design.md (2d739f5).

## Global Constraints

- Direct main, scoped ordinary commits/pushes; root owns ALL Git writes. No force-push, rebase, amend or history rewrite. One product writer; retain raw/SDD evidence.
- No vendor, dependency, license/notice, fixture or golden changes; no dependency installation or Windows/settings/security/automation actions.
- No UI writes/builds/GUI or UI/out recreation; preserve Avelabs712d454 and accepted Release EXE C92F26EE8FEB4FF03F6DC7F4AFFF4B11FB48142743D1EDCCB4E213AAA81F82C2.
- No public loader, Runtime handles, Player threading, Direct2D ownership, ANGLE/backend coverage or public fallback changes. New production interfaces are private/not installed; reference-linked install prohibition stays.
- No changes to legacy NativeEllipseStream except the identical private helper extraction in the spec review addendum; NativeEllipseCertificate, own reader/admission/model policy and existing oracle/goldens stay unchanged.
- No full Lottie, own playback/GPU, universal scalar parity, TSan, zero-allocation or measured-speedup claim. Samsung is configure/provenance-only in this stage.

## Review Focus

1. Duplicate identities can silently share history: Task1 shared-planner A/B and concurrent allocation, packed-handle/duplicate-ID negative characterization.
2. Late invalid viewport or exhaustion can corrupt retained results: Task1 success-failure-success history, counter boundaries and owned snapshots.
3. Visibility changes can hide stale native geometry: Task2 half-open activity and mixed seeks; Task3 old-plan replay after newer/inactive output.
4. Comparison normalization can erase defects: Task2 independent side validation and targeted mutation witnesses, not candidate-derived expected data.
5. Identical hashes do not prove owner life or producer safety: Task1 released source/prepared owners, Task3 retained draw/domain recreation; own-only domains documented as precondition.

## File ownership and environment

Task1 creates src/render/OwnNativeEllipseStream.hpp/.cpp and
OwnNativeEllipseStreamCounters.hpp; tests/own_native_ellipse_stream_tests.cpp and
tests/support/OwnNativeEllipseStreamTestData.hpp. Task2 creates separate tests/
own_native_ellipse_stream_differential_tests.cpp and support/
OwnNativeEllipseStreamComparison.hpp. Task3 extracts test-only
support/WarpCaptureSurface.hpp from direct2d_capture_tests.cpp, adds
tests/own_native_ellipse_stream_capture_tests.cpp. Each owns its CMake registration
while active. No public headers. Root owns docs, ledger, ALL Git writes and broad
final gates. Workers do not spawn subagents or write Git; return ready-for-commit.

Use VsDevCmd.bat -arch=x64 -host_arch=x64 before CMake/CTest (installed VS2022
BuildTools). Raw paths out/part26h/task-N, unique RED/GREEN attempt filenames;
never overwrite prior evidence. Missing API/compiler is not functional RED:
provide compiling unsuccessful stubs, run meaningful assertions, then implement.
Worker reports command, exact result/count and raw path; do not print whole logs.
No simultaneous builds/tests. All worker reports in this plan's ignored SDD dir.

### Task 1: Own stream, identities and independent lifecycle proof

**Files:** Task1 paths above plus CMakeLists.txt. Reuse existing own-model test
inputs and immutable factory chain, no fixture edits.

**Interfaces:** in avemotion::render::detail:

```cpp
enum class OwnNativeEllipseCreateCode {
    Ready, InvalidPreparedAsset, InvalidSourceIdentity, IdentityExhausted,
    EvaluationPreparationFailed
};
enum class OwnNativeEllipseFrameCode {
    Emitted, SequenceExhausted, InvalidViewport, EvaluationFailed,
    UnsupportedNumericOutput, ModelApplicationFailed
};
// CreateResult: code, string message, unique_ptr<OwnNativeEllipseStream> stream,
// explicit bool. FrameResult: code, string message, optional<EvaluatedScene> scene,
// explicit bool. Same ownership idiom as legacy stream, distinct own types.
static OwnNativeEllipseCreateResult OwnNativeEllipseStream::create(
    std::shared_ptr<const OwnNativeEllipsePreparedAsset> prepared);
OwnNativeEllipseFrameResult OwnNativeEllipseStream::emit(
    std::size_t frame, std::size_t width, std::size_t height);
// Private counter helpers are production-used, not live-stream test setters:
std::optional<std::uint64_t> tryNextOwnStreamIdentity(
    std::atomic<std::uint64_t>& last) noexcept;
bool tryAdvanceOwnStreamSequence(std::uint64_t& last) noexcept;
```

Stream noncopyable/nonmovable, owns evaluator/workspace, prepared owner, fixed ID,
attempt sequence and previous successful fingerprints. Last counter starts0,
UINT64_MAX is the final valid issued value; subsequent calls fail unchanged.
Test support `prepareOwnEllipseForTest(const std::string&)` returns the sealed
prepared shared owner, through own reader/model/resource factories; throws test
failure on any error. Later tasks consume this helper and the stream API only.

- [x] Write compiling stubs and literal tests: Ready from own JSON, invalid null,
  static and animated source/default/layer/draw/path/paint facts, invalid handles,
  independent stable fingerprint checks, activity10<=frame<20, frame999=>60,
  four viewports, reverse/mixed/repeated access, zero viewport advances attempt
  but not history. Existing tiny animated collapse vs static rejection, TGS path.
- [x] Observe functional RED before implementation; retain exact output.
- [x] Implement own scene assembly with existing G binding/evaluator/generator/
  materializer/applyModel/fingerprint functions, no copied numeric algorithms.
  Comment own-only domain/single-writer/allocator lifetime at private API.
- [x] Add focused counter RED/GREEN tests (atomic MAX-1=>MAX=>failure; plain
  sequence likewise; no live counter reset). Test distinct same-owner, identical
  separately prepared and different-source IDs; destruction/recreation; four
  concurrent workers each constructing/emitting eight streams with local outputs.
- [x] One shared planner tests A/B first/repeat/advance, static sharing/different
  source partition, retained snapshots/aliases/model after releasing all external
  owners, stale/equal/forget/rebuild, immutable source model. Synthetic copied
  scenes pin duplicate raw-ID and packed Runtime-handle collision behavior as
  characterization, not supported own/reference mixing.
- [x] `cmake --preset windows-msvc-direct2d`, build same preset --parallel4,
  `ctest --test-dir out/build/windows-msvc-direct2d --output-on-failure` in VS
  environment: full suite including new `avemotion.own_native_ellipse_stream`.
  Self-review, report raw evidence. Root verifies, normal scoped commit,
  independent task review, docs update and guarded ordinary push.

### Task 2: Independent ordinary-reference scene and plan parity

**Files:** Task2 paths above and CMakeLists.txt. No product changes unless an
observed defect is routed to root first. Existing exact comparators/oracle unchanged.

**Interfaces:** consumes Task1 API/helper; new test-only normalization/comparison
helpers consume original own/reference scene and original plans plus each side's
own source binding/schema. Their names are local to this test; Task3 does not
depend on normalized results. They return explicit mismatch diagnostics.
The unchanged NativeEllipseOracle header's legacy inline helper needs the
NativeEllipseCertificate.hpp declaration included first in TEST code; do not
construct a certificate or call that legacy normalizer. freshScene emits raw
instanceId0, so assign a fixed nonzero TEST oracle ID (independent of candidate)
before its separate planner build; keep its original asset handle/sequence/model.

- [x] Write positive equal-semantic assertion against a compiling false comparator
  stub, observe RED. Define fixed normalization exactly per spec in the test
  helper comment before implementing it. Independently validate original IDs,
  handles, resource roles/counts/aliases; no copying candidate values to oracle.
- [x] Use unchanged NativeEllipseOracle for all15 NativeEllipseTestAssets, four
  viewports (512x512,256x256,384x256,256x384). Per source/viewport compare forward
  0..60, reverse60..0 and [0,30,30,60,10,20,19,0], then mixed viewport sequence.
  Each original stream/oracle history has its own planner. Compare all retained
  semantic scene fields and plan fields, preserving revision/updates/history.
- [x] Implement normalization of approved identity/display-role differences only;
  original scenes feed planners. Compare unchanged exact scene/plan utilities.
  Pin original fingerprints, sequence, successful-history behavior separately.
- [x] Mutation witnesses reject geometry, color, transform, visibility, resource
  revision, unused gradient/image/stroke fields, bounds and wrong original source
  role. Live reference Runtime counters unchanged across own-only calls. Emit
  explicit comparison/sample/witness counts and matrix summary.
- [x] `cmake --preset windows-msvc-telegram-debug`, build same preset --parallel4,
  `ctest --test-dir out/build/windows-msvc-telegram-debug --output-on-failure -R
  "(own_native_ellipse|native_ellipse|own_json)"`: include the new
  `avemotion.own_native_ellipse_stream_differential` and related old/own tests.
  Retain RED/GREEN full commands/counts, self-review/report. Root verifies,
  scoped normal commit, independent review, docs/guarded push.

### Task 3: WARP pixels and backend lifetime/cache integration

**Files:** Task3 paths above and CMakeLists.txt. Test-only WarpCaptureSurface
extraction preserves the existing capture test's helpers, dimensions and behavior.

**Interfaces:** consumes Task1 stream/helper and unchanged NativeEllipseOracle;
shared testsupport::WarpCaptureSurface keeps configure(profile), context(),
begin(), end(), readPixels() behavior; no public consumer. No Task2 normalizer
in pixel oracle.
As in Task2, declare the unchanged oracle header's legacy types in TEST includes
and assign its raw scene a fixed nonzero TEST ID for its separate planner.

- [x] Extract the existing test-only WARP surface mechanically; build/run existing
  `avemotion.direct2d.capture` to prove unchanged behavior before new assertions.
- [x] New capture test observes functional RED via a compiling unset candidate
  draw step, then uses own plans to draw. Keep original ordinary reference
  scenes/plans in distinct planner/backend domains. Compare exact WARP BGRA bytes;
  independent ordinary CPU raster uses unchanged PixelComparisonPolicy.
- [x] Six assets: base animated, static-visible, activity10..20, nonlinear easing,
  nonsquare640x360, fractional-coordinates from existing15case matrix. Color
  boundaries remain covered by the full15case semantic matrix in Task2.
  Frames [0,10,19,20,30,60] x all five existing CaptureProfiles =180 cases.
  Own/reference scenes use logical viewport; CPU uses physical pixel dimensions.
  Empty-case assertions and nonempty control prevent vacuous success. Write raw
  metrics and mismatch images to a fresh test output location; no goldens.
- [x] Test own-only backend A/B/A with one own planner, static sharing, distinct
  animated slots, repeat cache hit, retained older plan after newer/inactive,
  source/stream destruction and forget; clear/recreate resource domain then draw
  retained plan. Assert actual cache diagnostics and equal retained pixels.
- [x] Register only when Direct2D capture and Telegram are enabled. Configure/build
  windows-msvc-win32-preview and run `ctest --test-dir
  out/build/windows-msvc-win32-preview --output-on-failure -R
  "(own_native_ellipse|direct2d|win32)"`; no product backend edits.
  Self-review/report exact coverage, root verification/scoped commit, task review,
  docs/guarded push.

## Root final gate and handoff

- [x] One whole-stage reviewer of base2606632..completed product, deferred minors
  included. At most one combined final fix wave and scoped re-review.
- [x] Fresh full configure/build/CTest none, Telegram, windows-msvc-win32-preview
  sequentially, exact HEAD/source hashes recorded with raw output. Existing
  out/part26g gate instruments may be adapted into out/part26h, never overwritten.
- [x] Samsung configure/listing only; existing all-vendor and TGS16 provenance;
  actual new stream MSVC include trace, link/install/private no-ref closure;
  no inherited G trace claim as proof of new source. No private headers installed.
- [x] Check unchanged host Git/EXE/UI-out. Report supported bounded semantics,
  counters/lifecycle/pixels and limitations; no benchmark or hardware claim.
  Update STATE, durable ledger, SDD and docs/PART26H_OWN_ELLIPSE_STREAM_REPORT.md;
  docs-only seal, guarded ordinary push. Preserve evidence; no automation action.

## Self-review and execution choice

Spec coverage maps to Tasks1/2/3 and final root gate. Public/private ownership,
counter signatures, comparison exclusions and file consumers agree. Review Focus
cases are assigned above. Controller approves written plan and selects SDD under
delegated authority, with one writer and root-only Git. No user reply is required.

## Task1 fix round1 addendum

Create src/render/NativeEllipseEvaluationHelpers.hpp with the exact two helper
interfaces and operations in the spec addendum. Replace duplicated resolution and
aspect-fit/finiteness blocks in own and legacy streams with helper calls only.
Add literal fingerprint witness(s) and independent derivation scratch evidence;
no expected values sampled from production. Functional helper RED then GREEN;
none full suite and existing Telegram native stream/lifecycle focused regression
tests. Root owns normal commits and scoped re-review. This adds the helper and
legacy stream.cpp to the authorized final private closure diff, nothing else.
