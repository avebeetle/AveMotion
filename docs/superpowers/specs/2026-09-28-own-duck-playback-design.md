# Part26N — unchanged Duck think in the working Avelabs UI

Status: controller-approved under the user's explicit delegation of written
design, plan and execution decisions. Approval is the controller's decision,
not a claim that the user reviewed this new document. Date: 2026-09-28.

## Outcome and scope

The user wants real downloadable Telegram animations in the future AveVoice UI,
not a succession of synthetic demonstrations. The acceptance target is unchanged
Duck think `a-3.tgs` through own AveMotion in
`D:/rvc/c++/DragonianVoice/Avelabs-UI/build/Release/AvelabsUI.exe`, using the
existing load/play/pause/seek/multiple-instance controls. It must not obtain
pixels or scene data from rlottie in the product. Synthetic fixtures are focused
regressions, not the delivery milestone. No full Lottie/SVG claim follows.

Pinned source, exact hashes and diagnostic baseline:
`docs/REAL_STICKER_BASELINE_2026-09-28.md`. Target encoded SHA-256:
`7CD55718288EB1B1D1EDFD0E774D9038E076A7F5AADF9835A92CF942948A4BC1`.
The original is local evaluation data at
`out/real-stickers-2026-09-28/assets/a-3.tgs`; decoded JSON is 60,315 bytes.
No original sticker, extracted artwork, decoded JSON or generated capture is
committed or bundled. SDK licensing is not independent artwork clearance.

The installed Part26L EXE remains SHA-256
`93B10E5C93A3182BC59A4333F5444E95F4D4CA32D506AA05E26D1F740C5C1645`
until a verified candidate is promoted with a complete backup. Its existing
26-file package, notices, corresponding PDB and rollback evidence are preserved.

## Alternatives and decision

1. **Selected:** bounded own compiler into the existing canonical model,
   retained evaluator and one shared scene program consumed by the existing
   stream/Player/WARP route. Adds only missing import and topology semantics.
2. Re-enable reference CPU for this sticker: faster visible demonstration but
   does not satisfy the own-engine delivery goal; not selected.
3. New generic renderer/runtime or complete Lottie parser: far broader than one
   measured input, duplicates working scheduling/rendering; not selected.

Read-only inventories are retained at `out/part26n-design/duck-feature-inventory.md`
and `reuse-design.md`. Existing canonical types already cover the required
tracks, paths, transforms, paints and trims. `SourceGeometryProjector` is not a
standalone scene generator: it verifies paths against an existing oracle scene.
Reuse only its pure materialization mechanics, not its oracle admission checks.

## Exact admitted vector subset

Accept a structural, value-validated subset, never an asset name/hash special
case. Existing primitive and strict legacy admission contracts remain intact.

- Root 2D composition, `ip=0`, integer `op` 2..10,000, positive finite `fr<=240`,
  dimensions 1..4,096. Normal blend, no effects/masks/mattes/3D/auto-orient.
- Shape and null layers, plus at most one root precomp instance referring to
  one asset, with no nested precomps. Precomp must have identity transform,
  full opacity, matching canvas dimensions, `st=0`, `sr=1`, no remap. Its clip
  is the final canvas. All child layers also have `st=0`, `sr=1`.
- Unique layer IDs, valid in-composition parent IDs, no self/cyclic parents.
  Resolve IDs, not array positions. Keep authored order and source provenance.
  Layer visibility is half-open and intersected with containing precomp; a
  transform parent being invisible or opacity zero must not hide its children.
- Each shape layer has one group containing one `sh`, optional solid `st`,
  optional solid `fl`, then `tr`; an unpainted path is allowed without a draw.
  The authored paint order determines back-to-front draw order. No nested
  groups, gradients, dashes, repeaters, merge paths, text or images.
- Static and animated open/closed cubic paths with equal `i/o/v` lengths,
  relative handles, at least 2 vertices, fixed vertex count and closure across
  morph keys. Preserve the existing evaluator's pinned morph closure semantics;
  do not change it globally or silently close an open stroked contour.
- Static opaque RGBA solid colors; static paint opacity 0..100; winding or
  even-odd fill; static stroke width (0,1,024], cap flat/round/square, join
  miter/round/bevel, finite miter limit [1,1,024] with Lottie default 4.
- Group transform is static translation, neutral anchor/scale/rotation/
  opacity/skew. Layer anchor is static; layer position/rotation/scale/opacity
  may be static or keyed. Position/anchor Z must be 0; scale Z must be 100.
  Omitted components use Lottie neutral defaults and must be tested.
- Optional one layer-level trailing `tm` after the group, mode 1, static zero
  offset, keyed or static start/end percentages in [0,100]. It affects that
  group's single path in the translation-preserving local space, before the
  layer transform. No general nested modifier engine is implied.
- Keyframes support multiple adjacent segments, terminal `{t,s}`, explicit
  hold `h=1`, scalar/Vec2/path values, temporal Bezier handles as scalars or
  equal-channel arrays, and paired spatial position tangents. Reject unequal
  channel easing, malformed terminal/ordering/topology, unsupported expressions
  or separated dimensions. Do not truncate curves to the root last frame.
  For this bounded profile both temporal control coordinates x/y are [0,1];
  overshooting temporal y is explicitly unsupported and rejected, not clamped.
  This meets the measured Duck input without introducing broader curve coverage.

Strictly validate every semantic field; accept only explicitly listed metadata
(`nm`, `v`, `tgs`, neutral `hd/bm/ddd/ao`, path `ind`, and property indexing
metadata already accepted by the established reader/admission conventions).
Unknown semantic fields reject with an actionable JSON-pointer diagnostic.
Do not silently discard masks, effects, timing, skew or non-neutral transforms.

Resource bounds: unchanged TGS decoder and 1 MiB JSON byte limit; default
`readOwnJson(bytes)` remains 4,096 values/depth32. A separate explicit vector
read profile admits at most 65,536 values/depth32. Parser remains iterative;
preserve duplicate/malformed-input precedence and byte-bounded memory behavior.
Compiler limits: 128 expanded layers, 128 paths, 256 draws, 256 vertices per
path, 256 keys per property, 4,096 properties, 8,192 total segments and 65,536
stored canonical shape points. Use checked arithmetic and reject before
expansion exceeds a bound. Property component magnitude <=32,768, scales
[-1,000,1,000] percent, opacity [0,100], key times [0,20,000]. Numeric conversion
must be finite and bounds-checked; discrete IDs/dimensions/times requiring
integers cannot silently round into range. Preserve existing exact primitive
numeric gates. No unbounded recursive walk or allocation guarantee claim.

Task2 implementation clarification: expose a private numeric-token conversion
helper from `NativeEllipseAdmissionCore.hpp/.cpp` for the new vector compiler.
It reuses existing JSON-number validation, exact decimal normalization/domain
and integral checks before finite conversion. No duplicate decimal parser,
public API or change to established primitive admission. Focused regressions
must reject rounded-into-range/fractional integer inputs and extreme exponents.

Duck's measured subset is 36 precomp child layers (32 shapes, 4 nulls), 31 parent
edges, 32 paths, 21 fills, 15 strokes, 7 trims and 33 animated properties. Direct
JSON has 303 adjacent intervals; the reference exports 304 segments because of
its key handling. Investigate that difference (held terminal candidate) rather
than imposing a false equal-count test. Compare evaluated behavior at boundaries.

## Architecture and ownership

Add private `OwnVectorModel` compiler over `OwnJsonDocument`. It owns one
immutable canonical `MotionAssetModel` plus binding IDs and visibility/order
metadata; numeric curves are not duplicated in a second runtime model.
Eligible precomp instances expand into one evaluation composition: preserve a
structural container, clone source IDs, resolve transform parents within it.
Structural ancestry controls opacity; layer transform-parent edges control
matrices only. This avoids introducing a second clock for an identity-clock
input. General time-remap/transformed-precomp support stays rejected.

Preparation lowers primitive or vector bindings into one immutable
`OwnSceneProgram` with layer intervals, ordered draw/path/paint/trim IDs and
resource classification. Existing `OwnNativeEllipsePreparedAsset` gains a
vector owner alternative and program; its historical private name need not be
renamed across the project. Existing primitive factory API remains callable.
New `prepareOwnMotionAsset(document)` selects a profile structurally once,
then returns the same prepared-owner result family with useful diagnostics.
There is no renderer retry or hidden fallback on admission/render failure.

One existing `OwnNativeEllipseStream` evaluates one workspace, materializes
program draws with shared shape/primitive/trim helpers, emits actual layer
ranges and separated transforms/opacity, applies the model and computes scene
history. It must not retain two complete frame emitters. Preparation and frame
failure publish no partial owner or scene. Stable local resource IDs/classes,
single-writer identity/sequence, ownership/retirement and independent-stream
contracts remain. Static resources only when all local dependencies are static.
Changes of layer/group matrices remain separated where the planner supports it.

Playback reads metadata from the common prepared model, preserving existing
float-rounded `(N-1)/fr` and frame mapping. No new scheduler/worker/timer/public
Runtime handle, no public fallback-policy change, no backend ownership change.
Host preparation alone switches to explicit vector reader/new preparation;
existing Player, worker, canvas and WARP readback remain. WARP is software
rendering; no hardware GPU performance claim.

## Reconcile the frozen work

The 22 uncommitted M Task2 source/test files are retained as a prerequisite,
not discarded, silently certified or repeated as another host milestone.
First reproduce and fix multi-resource canonical source keys against ordinary
reference mapping (model ID+1, singleton still 1), run the blocked differential,
full none/Telegram suites and independent review of the whole preserved delta.
N owns acceptance of that carried delta; M's old partial report stays historical.
M's separate synthetic host/capture/promotion sequence is superseded, not done.

## Evidence and acceptance

Functional TDD precedes each production change; independent task reviews and
whole-stage review precede promotion. Root owns Git, directly on main by user
choice. Keep raw failures and successful evidence; no test/golden weakening.

1. Committed first-party focused regressions exercise parent opacity separation,
   hold/terminal timing, spatial tangents, open/closed morph, paint ordering,
   local outer trim and rejection boundaries. External Duck is loaded from an
   explicit test input path; missing local artwork is not a passing real test.
2. Early own compile of original TGS, then all 180 forward/reverse frames,
   repeated seek, frame179/loop boundary, viewport round trip, two isolated
   streams, 1/4/16 host instances, stale completion and owner retirement.
3. Independent ordinary Telegram reference compares all visible draws in order:
   contours/verbs/closure, colors, stroke metrics/caps/joins, opacity and world
   transforms. Compare authored/evaluated properties by roles, not raw IDs from
   intentionally flattened composition. Exact topology/enum/color fields;
   numeric tolerance at most 1e-4 logical pixel or scalar absolute error, with
   reported maxima, not a tolerance increased to hide errors. Resource identities
   and history are tested within each own stream. Mutation witnesses must catch
   omitted trim, parent-opacity inheritance, paint reordering and changed curves.
   One pinned-reference inactive-field difference is explicit: for an authored
   stroke with omitted `ml` and round/bevel join, own default4 versus reference
   default0 must be asserted and counted as that exact pair, not erased or
   loosely tolerated. Miter joins and any explicit `ml` retain normal equality;
   a mutation must prove active-miter differences fail. Rendered parity remains
   required. Shared first-party oracle fixtures explicitly state `ddd:0` before
   `ks` because this pinned parser otherwise defaults3D; own omitted-neutral
   defaults retain separate hand-derived tests. Original Duck bytes never change.
4. Same-backend WARP comparison at frames0,10,15,20,45,90,110,135,179 and
   viewports128/256/512; exact pixels where equivalent input arithmetic permits.
   Record any nonzero image diff and investigate; CPU-vs-WARP uses only the
   existing capture policy, never a new relaxed threshold. A policy-blocked
   ordinary plan is not bypassed silently: record a bounded test-only reference
   materialization design before adapting it. No performance conclusion from
   incidental test durations.
5. Full MSVC none and Telegram suites, explicit `windows-msvc-win32-preview`
   capture/WARP/device recreation, no-reference product boundary and existing
   host reference/own/working-entry gates. Samsung remains optional comparison;
   its known unrelated Polystar failures are not called all-green.
6. Fresh static Qt6.10 /MT candidate, source/link/import/notices/PDB provenance,
   actual own Duck load/play/seek UI captures, then recoverable promotion to
   canonical `build/Release`. Recheck process ownership before replacement.
   Do not close the user's process. Native desktop/DPI/tray acceptance is
   distinct from Qt test-page screenshots and is claimed only if exercised.

Native acceptance clarification (controller decision under delegated reversible
UI-test authority): use ordinary owned-process interaction with the actual EXE.
It is not settings-isolated: normal Open/Exit may persist Avelabs window geometry,
Qt file-dialog history and ordinary bounded logs. Those inherent application-use
side effects are permitted for this requested test; do not manually edit or
restore registry values, change Windows preferences, add a product test hook,
or terminate a user process. Avoid unnecessary window/DPI/layout changes. Exact
write paths and procedure are recorded in the native-acceptance audit. This
clarifies normal UI use, not authority to modify system/security settings.

## Exclusions and stop boundaries

No system dependencies/settings/drivers/security changes, no vendor or golden
edits, no external artwork publication, no GitHub Actions or old-worktree code,
no force push/rebase/history rewrite. A foreign remote change stops only push.
Preserve reference-linked installation prohibition. Host build artifacts belong
under `build/cmake`, `build/checks`, `build/symbols`; host `out` stays absent.
Do not create/resume/change an automation in this stage without a new request.
If unavailable native tooling prevents final GUI evidence, report it honestly
and retain the verified candidate; do not label the end-to-end milestone done.
