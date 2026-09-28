# Part26O — unchanged Duck X3 through own playback

Status: controller-approved under the user's explicit delegated design/execution
authority. Approval records a technical decision, not a claim that the user read
this document. Engine baseline04c4a7b; host baseline799c7c4. Canonical Part26N
remains installed and protected while this stage is built and verified.

## Outcome and scope

Play the unchanged official Telegram example `a-2.tgs` through the existing own
AveMotion pipeline in `D:/rvc/c++/DragonianVoice/Avelabs-UI/build/Release`.
The already accepted `a-3.tgs` remains a regression. Do not substitute a synthetic
asset, change the original, silently fall back to rlottie, or declare support
from successful parsing alone. No general Lottie completeness or a-1 milestone.

Pinned input: TelegramMessenger/TelegramStickersImport commit
`b8951c8a02b245142142d341644d0756cf16278c`, Example/Source/a-2.tgs;
SHA256 `2CDBF4D03DE421B6E0BC73871F4FC7ABB685EF3B3B30B581E8AB75C71501B15A`.
Local evaluation only; artwork stays under ignored out and is not redistributed.
The own implementation uses ideas verified against existing pinned Telegram
rlottie `67f103bc8b625f2a4a9e94f1d8c7bd84c5a08d1d`, without copying vendor code.
Read-only design evidence: `out/part26o-design/duck2-inventory.md` and
`out/part26o-design/precomp-architecture.md`. These distinguish measured facts
from implementation proposals and do not themselves prove playback.

## Measured requirements

The original is5397 compressed bytes/52883 decoded bytes,9736 JSON values,
512x512,60fps,180 frames. Existing explicit vector-reader limits suffice;
do not raise the default reader or transport limits.

- Two assets and repeated nested references expand to50 layer instances,
  including6 precomp instances. Two inner instances are simultaneously visible
  at different local frames. Source definition identity is not instance identity.
- Root frames0..127 use comp_0 local52..179;128..179 use local0..51. Nested
  precomp st values86 and10 produce additional offsets. Shape st=-5 is metadata,
  not another subtraction from its property clock.
- Precomp transforms need only static translation (+12,+9 on inner instances),
  neutral scale/rotation/full opacity, and sr=1. Every enclosing clip matters.
-18 one-level groups contain18 fixed-topology Bezier paths, solid fills/strokes,
  inline and trailing simultaneous trim; one layer has two groups followed by
  shared trim and two layers have a top-level stroke following a filled group.
- Three stroke widths animate through zero; one group has static uniform scale
 75.153%; two trim tracks start at-12. Fractional keys and easing overshoot are
  authored, including differing easing on an otherwise neutral Z scale channel.

## Architecture

Extend the existing compiler -> immutable canonical model/program -> retained
PropertyEvaluator/workspace -> own stream -> Player/Qt worker/WARP canvas.
No second runtime, worker, scheduler, cache subsystem, bitmap fallback or renderer.
Keep historical private type names rather than broad renaming.

### Instance expansion and clocks

Expand each asset reference occurrence into distinct source/property/draw IDs.
Resolve authored transform parents inside that occurrence, independently from
structural precomp containment. Detect dangling/duplicate IDs, sibling parent
cycles and asset-reference cycles; repeated acyclic references are legal.
Preserve authored IDs, source pointers/instance ancestry and authored ip/op/st/sr
as provenance. Flatten execution into the existing single evaluation composition.

Admit up to8 asset definitions and4 nested precomp levels, counted from the root
composition. Existing global128 expanded-layer/128 path/256 draw/4096 property/
8192 segment/65536 canonical-shape-point budgets remain. Check expansion before
allocation, including inactive branches. Reject unused assets as before. Signed
integer layer ip/op/st are bounded to[-20000,20000], op>ip, sr exactly1; root
ip remains0. Accumulated child-clock offset is checked and bounded to80000.

For each expanded occurrence, preserve authored keys and store an immutable
per-property integer clock offset. Add an explicit offset-binding overload to
the existing evaluator; its ordinary constructor retains zero-offset semantics.
Empty binding means zero; nonempty binding must match property count and bounded
values, copied/owned by the immutable evaluator. Evaluate localFrame=rootFrame-
offset consistently for segment lookup, interpolation, spatial and shape values,
cursor state and history. Never shift fractional authored keys into root time.
The own stream binds these offsets; ordinary reference/parsed-model clients do
not infer clocks from their source st fields. One workspace per stream remains.

A precomp's own properties and visibility use its containing composition clock;
only its children subtract that precomp's st. Shape/null st does not shift keys.
Map half-open layer visibility to root time and intersect all structural ancestor
intervals and root playback. Preserve invisible/zero-opacity transform-parent
matrices without treating that parent's visibility/opacity as structural ancestry.

### Shape scope, animated stroke and numeric admission

Extend only the measured one-level group grammar. Maintain authored traversal,
paint stacking and explicit associations of each paint/trim to its path(s), not
an unordered flattened set. A top-level stroke may bind preceding group paths;
inline trim and trailing sibling simultaneous trim must follow pinned reference
scope and transform order. Scope semantics get a written addendum from the
read-only audit before Task2 production edits. Do not silently admit arbitrary
multiple modifiers or unsupported mixed transforms; fail closed when not proved.

Static group position and positive uniform scale are sufficient here; keep
anchor0, rotation/skew0, opacity100. Animated solid stroke width samples the same
retained evaluation view and local clock, never a fabricated static value. Width
zero is valid; exact reference publication/raster behavior must be characterized
before choosing its representation. Final-space stroked geometry remains
instance-evaluated. Keep colors/paint opacity static as in N.

Allow finite key times[-20000,20000], strictly increasing after the existing
numeric representation checks. Temporal X controls remain[0,1]. Bound temporal Y
to[-16,16] only where evaluator behavior is proved; allow ignored neutral Z ease
only when the entire Z value/tangent contract is neutral and XY ease agrees.
Unequal visible XY easing remains unsupported. Preserve exact decimal admission,
finite output checks and explicit rejection of unsupported fields.

### Clipping and publication

Replace Boolean precomp membership with immutable enclosing-instance clip
bindings. Admit static translated, full-opacity precomps with integral width/
height1..4096 and no authored transform parent; no affine/animated precomp
transform, stretch, remap or nonunit precomp opacity. For each emitted draw,
prove conservative raster footprint intersect target is contained in EVERY
enclosing mapped rectangle. Use actual float viewport boundaries and current
stroke/AA/rounding padding. This is a redundant-clip proof, not general clipping.
No approximate equality waiver or assumption that square target removes inner
left/top clipping. Empty geometry and disjoint target intersections are explicit.
Failure produces no partial scene, no successful-history update and no fallback.
If a-2 actually needs clipping, retain rejection, capture failing frame/bounds,
and approve a bounded revised design before proceeding.

## Acceptance and exclusions

### Shape-scope design addendum (approved before implementation)

The local pinned `lottieitem.cpp` processPaintItems/processTrimItems traversal
collects earlier authored paths recursively, scoped by its group-entry offset.
All path updates precede trim application, which precedes paint materialization.
Thus both inline trim placements in a-2 trim the raw local path before its group
transform. A trailing sibling simultaneous trim applies separately to each
preceding path, not their combined arc length. Admit at most one effective trim
per path; reject compound modifier chains until separately implemented.

Give each draw distinct path-transform and paint-transform bindings. An inner
paint uses its group for both; a top-level stroke uses each path's group for
geometry but its containing layer for width/opacity. Preserve paint operator
stacking across all bound paths; do not apply group scale to a layer-level pen.
Positive uniform static group scale may range(0,1000]; other group restrictions
above remain. Keep each draw/resource identity distinct, even when path is shared.

Pinned parser interpolator handling uses the first channel. The bounded own
profile still requires equal visible XY controls and exact-neutral Z values;
it may validate and ignore differing bounded Z controls without changing XY.
The existing evaluator already permits out-of-unit temporal-Y interpolation;
only compiler admission should widen, with negative/overshoot witnesses.

Reference retains an enabled stroke with width0 and zero CPU coverage. Do not
turn it into a fill or discard its draw identity. Verify WARP width0 explicitly;
if the existing backend differs, isolate a narrow behavior-preserving correction
with RED and review rather than admit a known misrender. Full audit references
are retained in the read-only architecture note.

Functional RED witnesses precede each production block. First-party fixtures
carry reusable regressions; originals remain local. Independent task reviews and
whole-stage review precede promotion. Oracle roles must distinguish complete
instance ancestry plus authored source role; paint source IDs alone collide.
Reference clip expectations must derive independently from reference ancestry,
dimensions and transforms, not own bindings. Preserve N raw local cancellation
evidence/rules and all existing final-space/pixel tolerances; no golden edits.

Compare every frame0..179 at128/129/256/512 square targets and reverse/seek/
viewport histories; emphasize34..67 overlap and67/68,127/128,137/138 boundaries.
Capture own vs fresh ordinary-reference WARP at0,33,34,67,68,127,128,137,138,179
at those targets, plus existing CPU acceptance policy and full a-3 regressions.
Run full no-reference, Telegram, windows-msvc-win32-preview gates, affected host
working/opt-in/reference tests, own-worker real-file tests and actual product
no-reference/static-link audit. WARP remains software, not hardware proof.

Build a fresh frozen-source /MT host candidate, verify EXE/PDB/notices and bound
audit evidence, preserve a full N backup, and promote only when no user process
locks canonical artifacts. Never terminate user applications. Check native
load/play/pause/seek/count/visibility in the canonical UI when tools allow;
record manual/DPI/hardware/stress limitations honestly. No claims of speedup,
zero allocation, race sanitization or completion without evidence.

User-authorized main commits/ordinary pushes only after scoped tests/review and
remote checks; one product writer, preserve raw/SDD and foreign edits. No force,
rebase, dependency installs, Windows changes, licensing decisions or new schedules.
Host artifacts live under build; host out stays absent. a-1, live scrubbing,
UI redesign, gradients/masks/mattes/images/text/repeaters and general compositing
remain outside this stage.
