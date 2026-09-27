# Part26M — bounded own primitive groups in the existing UI

## Intent, authority and success

User requested the next useful own feature against a checked example, chose
controller selection and authorized the canonical Avelabs EXE, small commits
and ordinary pushes. Written decisions/approval/execution are delegated.
This is an architectural extension of private typed data, controller-approved
under that delegation, not a claim the user read this file. Bases after L:
engine1f2f601 and host0d56e83. Work in existing main; preserve raw/SDD.

Success: unchanged first-party `tests/fixtures/primitive_geometry.json` and
manifest-pinned `tests/compatibility/tgs/primitive_geometry.tgs` render all
seven shapes, including both animations, through the existing own pipeline
and Avelabs Voices/Motion Lab. Install a reviewed verified update into the same
`build/Release` with backup. No fixture-name/hash special cases or dropped
unsupported groups. This280x230 technical fixture is not a downloaded real
Telegram sticker; the rights-cleared real-sticker milestone remains open.

## Approach and exclusions

Generalize the existing own model/preparation/stream to a bounded ordered list
of filled primitives. Reuse OwnJsonReader, exact-decimal validation/conversion,
PropertyEvaluator, PrimitivePathGenerator, Player, worker, timer, QImage canvas
and WARP backend. Preserve the separate reference-certified ellipse APIs.
Do not create a parallel rectangle renderer, second playback stack or reference
fallback. Alternatives: exact-seven special casing is not genuine coverage;
rlottie fallback violates the own-only package; a general Lottie rewrite is
larger than this concrete step. No DLL/module framework, new scheduler,
dependency install, Windows change, vendor/golden edit or public license choice.

## New own-only admission contract

One2D shape layer with1..16 direct groups, subject also to current4096 value-node
limit. Each group is exactly `[el-or-rc, fl, tr]`, with optional inert names;
no strokes, modifiers, masks, nested groups, images, expressions or unknown
keys. Preserve existing root/layer metadata, timing and neutral transforms;
only existing bounded static layer translation is allowed. Missing layer `ao`
means inertfalse in this new profile, explicit `ao` must be0. Legacy grammar
continues requiring its explicit0 and single clockwise ellipse.

- Both primitive directions `d=1/3`. Ellipse `p,s`; rectangle `p,s,r`.
- Position and size may be static or the existing one-segment/two-key shape.
  Radius may be static numeric or one-segment scalar with one-element endpoint
  arrays. Keys exactly0 androotOp-1, terminal start equals previous end by exact
  decimal comparison. Reject extra/hold/spatial/per-axis-different interpolation.
- Easing supports existing scalar x/y and component arrays of the property's
  dimension; for Vec2 both components must be exactly equal. Values remain0..1.
- Root dimensions1..8192, totalframes2..10000, frame rate(0,240], position and
  layer translation±32768, sizes(0,16384], radius[0,16384]. Preserve integer
  structural checks, color[0,1], alpha1, opacity100, fill rule1. Roundness clamp
  comes from existing geometry generation, not input rewriting.
- Preserve reader/transport byte/depth/node/string limits and fail-closed
  diagnostics with JSON pointers. Unknown/malformed/unsupported later groups
  reject the entire source. Exact-number acceptance and float representability
  remain distinct; finite/underflow failures must not publish an owner.

## Typed transition, one authoritative owner

Add private `OwnPrimitiveInput` with common composition/layer fields and
`vector<OwnPrimitiveGroupInput>`; each group owns kind/direction, exact-decimal
static-or-segment position/size/radius, fill and optional names. Add corresponding
`OwnPrimitiveNumericValues` (frameRate/translation and per-group numeric values)
and `OwnPrimitiveBinding` (common source IDs plus per-group property/source IDs).
Share the existing bounded value-table projection and decimal helpers. Do not
duplicate an entire JSON parser/auditor or maintain a shadow legacy input in
the production owner. Extract private common helpers where necessary.

Keep private `OwnNativeEllipseModel`, PreparedAsset, Stream, Playback and Player
factory signatures to avoid a28-file naming-only rewrite. Change only the
model owner's input/values/binding payload types to the new canonical types.
The model factory uses new `decodeOwnPrimitiveInput`; existing
`decodeOwnNativeEllipseInput`, `evaluateNativeEllipseValues`, NativeEllipseInput,
legacy binder, certificate and scan retain their strict contracts. This is
one own route despite historical private names. Host text must say bounded own
primitives, not ellipse-only or full Lottie.

Build source tables and render resources by looping validated groups. Exactly
one composition, root+shape layer and N(group,primitive,fill) triples. For the
whole fixture:23 source nodes,22 source-child edges,48 properties,43 static and
5 animated properties/tracks/segments;7 geometry/paint/draw slots,5 static and2
instance-evaluated geometries,7 static paints and2 render layers. Validate all
IDs/owners/ranges/edges/value rows exhaustively, before and after preparation.
No valid prefix may escape on failure. Preserve old single-ellipse row IDs,
allocation order, fingerprints and static resource hashes. New groups are not
aliased merely because colors or shapes happen to match.

Existing one-ellipse tests retain their literal expectations through checked
test-only one-group adapters, never a generic count-only weakening. Keep the
legacy reference binder's reindex/mutation tests; add independent own binding
mutation checks. Production payload has no redundant old/new truth sources.
Reuse existing frame/viewport/lifetime/identity/active-interval behavior. A
frame evaluates its workspace once then emits the ordered group list; correct
per-item bounds, aggregate counts/fingerprints and source roles flow into the
ordinary planner/backend. Repeated frames/resource identity remain stable.

## Telegram checkpoint and differential gates

Pinned local TelegramMessenger/rlottie commit
`67f103bc8b625f2a4a9e94f1d8c7bd84c5a08d1d`. Relevant implementation:
`lottieparser.cpp`1236–1287(rc/el fields/staticness),2029–2111(key segments),
`lottiemodel.h`595(defaultao), `lottieitem.cpp`1355–1380(centered primitive
paths),1158–1172/1230–1255(reverse group rendering). Paths are below
`third_party/rlottie/telegram/source/src/lottie`. Existing first-party
PrimitivePathGenerator already implements these primitives and clamp.
This is a behavioral oracle, not a new vendor patch or copied runtime.

Production `none` build must not link/call Reference/rlottie. Separate test
binaries compare full field/path/scene/plan semantics with explicit identity
normalization, all fixture61 integer frames and direct/reverse access. Preserve
the strong existing one-item comparator and add multi-item comparisons. Prove
draw order directly and with overlapping geometry: separated fixture pixels
alone cannot detect a reversal. Unsupported mutations of the later groups,
0/17groups, dimensional/easing/radius numeric boundaries and bindings must fail.

Own and ordinary-reference scenes rendered through WARP must match bytes;
ordinary CPU comparison retains existing PixelComparisonPolicy thresholds
(IoU≥.78, alpha relative error≤.12, all/active mean absolute differences≤12/30,
large difference fraction≤.18, bounds delta≤4). Do not loosen policy or replace
goldens. Include frame0,15,30,45,60 and an active intermediate integer frame,
five existing capture profiles, lifetime/cache/resize checks and1/4/16 instances.
If parity fails, establish root cause and revise the bounded design, not output.

## UI, final gates and handoff

Same existing worker/Player/canvas and own-warp mode: JSON/TGS whole fixture,
play/pause/seek,1/4/16, visibility/lifetime, errors/replacement preservation,
owned QImage and truthful diagnostics. No auto-load of an unsupported asset,
new settings, backend switch or user-profile changes. Existing own ellipse
and reference UI tests remain. Capture test-page pixels for the whole fixture
and inspect them. Counts/resource diagnostics are observations, not claims of
zero allocation, speedup, native GPU acceleration or universal sticker support.

TDD, one product writer, independently reviewed tasks, one whole-stage review
and at most one combined final fix/scoped re-review. Fresh full MSVC none and
Telegram suites; explicit windows-msvc-win32-preview capture/WARP/device
recreation; applicable Samsung legacy/own targeted tests (known unrelated
Polystar failures not hidden). Host reference/own/working-entry Qt tests,
Python/build boundaries and fresh static app+smoke. Keep canonical L EXE/PDB
until frozen candidate/notices/imports/symbols and complete backup pass. Recheck
user process, never terminate it; install and recheck hashes/readiness/symbols.
Native desktop/DPI acceptance remains separate if no safe harness exists.
Only reviewed scoped ordinary pushes after remote checks; preserve all evidence.

Self-review: requirements are bounded to one demonstrable geometry increment;
legacy certificate and own-profile semantics are deliberately separate;
private type migration is explicit; no hidden fallback/license/asset decision.
Controller approves this spec under delegated authority, then writes the plan.
