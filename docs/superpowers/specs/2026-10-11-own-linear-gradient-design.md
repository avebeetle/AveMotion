# Own static linear gradient fill design

The next capability is bounded static RGB linear fill through the existing own
vector model, scene stream and Direct2D executor. It is a step toward unchanged
Noto Party playback, not Party acceptance: four alpha matte pairs remain separate.
The installed Avelabs Test Animation EXE stays unchanged until original-input
acceptance and the later host delivery gate.

## Intent and approval

The user wants general capabilities driven by real animations, a self-contained
AveMotion integrated statically into Avelabs, and measured optimization without
extra formats, schedulers or reference fallback. The controller selects and
approves this written design under the user's standing delegation of reversible
technical decisions, written spec/plan approval and direct main execution.
Base: `4acb7bb28c6cd5d3fc3e622d005cd41b5be007e2`, canonical source
`C:/Users/USER/Desktop/AveMotion`. This is architectural because own scene paint
and a private backend execution seam must change together.

## Corpus and source evidence

Local `out/real-corpus-2026-10-08/assets/noto-1f389.json` remains byte-identical,
SHA256 `EAB1FD7070BFFD2965C12A80D2D91CFF76683AF65019F142ACF9A553EE877B5C`.
Its only `gf`, in `layers[10].shapes[0].it[1]`, has `t=1`, winding rule,
opacity100, start `[126,-94]`, end `[-84,72]`, and three RGB stops at 0/0.5/1.
There is no opacity tail. Its parent has animated nonuniform scale; the parent's
zero opacity does not suppress a transform-parented child.

Pinned Telegram rlottie `67f103bc8b625f2a4a9e94f1d8c7bd84c5a08d1d` inverse-maps
pixels through the full gradient matrix before projecting on local endpoints
(`src/vector/vdrawhelper.cpp:805,300`). Its render-tree endpoint export is not
enough to recover that raster operation under nonuniform scale. Preserve local
gradient data and the full existing geometry/paint-scope affine transform.
Byte colors use truncation of `255*channel`; opacity composition and final scene
alpha must match the ordinary pinned reference, not a second arbitrary rounding.
No upstream code, vendor version or license changes are part of this feature.

## Approaches and selected boundary

A compiler-only change cannot render gradients: the current own sampler is solid
only and the public Direct2D executor skips gradients. Broadening the public MVP
support/fallback contract is not selected. A separate renderer would duplicate
resource ownership. The selected approach shares the existing backend executor
through a fixed private static-linear-fill entry; the ordinary public entry and
public planner's MVP unsupported accounting retain their current behavior.

The private entry validates actual selected local paint and feature contents,
not caller-set own/source flags. It preflights all plan items before issuing draw
commands for unsupported content. It supports existing solids/strokes plus the
bounded linear fill defined here, never image/radial/dash/clip/mask/matte or
gradient stroke. Backend resources still belong to the host-supplied context,
domain and generation; AveMotion does not own BeginDraw/EndDraw or presentation.

## Authored admission

Admit `gf` only where a group-local solid `fl` is already admissible, with the same
one-fill paint ordering. Do not admit outer gradient paint or broaden shape scope.
Allowed fields are `ty,nm,mn,o,r,bm,hd,g,s,e,t,h,a`. Existing metadata validation
keeps `bm=0`, `hd=false`, and bounded names. `t` is required exact integer1;
rule is absent/default1 or exact1/2. Endpoints are required genuinely static
finite two-dimensional properties in -32768..32768, distinct after conversion.
Opacity is static0..100, absent/default100. Optional highlight length/angle must
be genuinely static zero. Keyed endpoint, opacity, highlight and stop properties,
unknown fields, radial types and non-neutral operative fields are rejected.

`g` contains only required `p,k`. Stop count is exact integer2..16. Its `k`
property permits only `a,k,ix`, with absent/zero `a`, valid existing `ix`, and a
flat numeric array of exactly `4*p` entries. Every offset/R/G/B is finite0..1;
offsets are strictly increasing. No sorting, duplicate offsets, opacity tails or
implicit stop-count inference. Asset-wide gradient float storage is capped at
4096 before append. Existing source/property/layer/draw/track/shape/reader budgets
remain unchanged. Any rejected input returns no authored/prepared asset.

## Model and scene contract

Use existing `SourceNodeKind::GradientFill`, `SourceGradientType::Linear`,
`GradientStart/End/Opacity/Stops` and packed gradient value tables. Add no public
model/evaluator type. Internal `PropertyCompiler::staticGradient(owner, property,
stopCount)` produces one static GradientStops record; existing scalar/vec2
compilers handle the other properties. Statistics count the new gradient values.

Internal scene bindings name endpoint/stops properties explicitly. Sampling
produces static local gradient stops/endpoints with inactive solid/image/radial
fields neutral. Final scene endpoints and stop alpha match ordinary reference
export; local paint stays static and inherited opacity stays separated. Parent
layer opacity is not inherited through a transform-parent link. Counts report
gradient fills/stops, not solids. Animated geometry/ancestry remains supported
through existing clocks and matrices; endpoint/stop animation is not added.

Asset-static canonical paint and existing paint keys/hashes remain the resource
identity. Affine/presentation/opacity changes do not recreate static local paint.
No new timer, worker, registry or scene program is introduced.

## Private Direct2D execution

Add build-tree-only `backends/direct2d/LinearGradientBackendAccess.hpp` with
`detail::LinearGradientBackendAccess::draw(Backend&, const MotionRenderPlan&,
const RenderSession&) -> DrawResult`. A private friend/internal shared executor
is the only installed-header declaration change; public signatures stay intact.
The private entry accepts linear fills only with coherent local rendering space,
finite distinct local endpoints, 2..16 ordered stops, neutral radial/image fields
and no unsupported feature bits. Never infer eligibility from a claimed source.
Unsupported private plans return `UnsupportedFeature` before any draw call;
invalid references return `InvalidPlan`. Ordinary public draw keeps its old skip
contract, including its existing gradient rejection test.

Use local endpoints and the full existing geometry transform followed by
presentation and saved host transform exactly once. Create a pad/clamp gradient
stop collection and linear brush. Use `D2D1_GAMMA_2_2` for interpolation in encoded
sRGB space and verify a black/white midpoint witness, not brightness-corrected
linear-light interpolation. [Microsoft gamma documentation](https://learn.microsoft.com/en-us/windows/win32/api/d2d1/ne-d2d1-d2d1_gamma)
distinguishes these interpolation spaces. This does not promise exact byte
identity with rlottie's integer lookup table; unchanged CPU-reference capture
thresholds are the raster acceptance gate.

Cache stop collection/brush by existing paint slot/content/revision; keep opacity
and transforms outside static stop resources. Clear gradient resources on domain,
generation, factory or device-context change and Backend clear/invalidation.
Solid diagnostics remain solid-only; DrawResult counts all drawn items. Contract
fakes record gradient creation/use for lifetime verification, avoiding a new
public diagnostic API. Do not add an eviction policy or cache optimization here.

## Verification and delivery

One product writer at a time, functional RED before production edits, focused
GREEN and fresh independent task review. Task1 owns model/scene; Task2 owns private
backend/capture. Controller approves artifacts, commits scoped changes and uses
ordinary push after remote check. A foreign remote commit blocks only push.

Tests use owned geometry, literal expected local/final gradient values, fractional
opacity, transform-only parenting with parent opacity0, nonuniform scale plus
rotation and presentation, reverse/repeat sampling, static paint keys and malformed
input boundaries. Differential corruption witnesses must detect wrong endpoint,
stop position/color/alpha and inherited opacity. Same-backend WARP equality alone
is insufficient: independent CPU raster and an affine interior-pixel witness are
required, using unchanged tolerances. Invalid/unsupported private plans must not
draw a valid prefix. Resource failure/domain/context recreation and public skip
behavior remain separately tested.

Final gates are fresh full MSVC no-reference, Telegram capture and explicit
`windows-msvc-win32-preview` suites, original a-2/a-3 regressions, frozen bytes and
whole-stage independent review. Preserve raw failures/warnings/evidence; do not
change goldens or tolerances to pass. No installed UI/performance/hardware GPU
acceptance follows from these gates. WARP is software.

## Exclusions and costs

Strict bounds reject additional legitimate gradient variants rather than guess
their semantics. Alpha tails, animated gradients/endpoints, radial/gradient stroke,
outer paint and matte composition remain unsupported. The private entry preserves
public policy but requires later deliberate host wiring. Original Party and its
installed playback are not complete until alpha mattes, unchanged-input acceptance
and host gates pass. Keep artwork local with existing source/rights records;
no new download, publication, format, licensing, dependency or Windows changes.
