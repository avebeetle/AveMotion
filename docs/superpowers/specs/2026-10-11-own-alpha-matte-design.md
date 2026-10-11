# Own alpha mattes and original Party design

Status: controller-approved under the user's standing delegation, 2026-10-11.
Engine base: `2fbdb35dcb46d352b2bde137572120cbdef4ea02`; host base:
`e02c2eb4f641dd55fdf64a015eb0047277cc0dae`.

## Outcome and boundaries

Implement a bounded, reusable normal-alpha track-matte profile in the existing
own model/scene and private Direct2D executor. Prove the unchanged original Noto
Party before any deliberate Avelabs `build/Release` replacement. Do not build a
second renderer, scheduler, bitmap-mask pipeline or general blend engine.

- Canonical E is `C:/Users/USER/Desktop/AveMotion`; H is `D:/rvc/c++/DragonianVoice/Avelabs-UI`. Work directly in main; scoped commits and ordinary push only.
- C++20, existing MSVC2022 x64/Qt6.10.0, existing dependencies; static host CRT `/MT`, own host variant `none`. No dependency installation, Windows/security/automation changes or new licensing/publication decisions.
- Preserve public backend fallback/skip policy, goldens, tolerances, vendor sources/notices, host threading/mailbox/docking/DPI/tray behavior and existing budgets.
- The previous private linear-gradient entry remains available with its existing scope. New capability uses a fixed private alpha-matte successor; public gradient/matte/blend support is not enabled.
- WARP is software; correctness and UI testing are not hardware-GPU or speedup evidence.

## Witnessed source semantics

Pinned Telegram is `67f103bc8b625f2a4a9e94f1d8c7bd84c5a08d1d`.
`lottieitem.cpp` reverses authored sibling order, pairs `tt:1` content with its
adjacent provider, renders separate transparent premultiplied inputs, then
applies destination-in. Provider RGB is irrelevant; providers never render as
ordinary artwork. Both intervals must be active. Transform parenting is not an
opacity/pairing edge. Preserve those ideas without copying vendor code.

Original local Party is `out/real-corpus-2026-10-08/assets/noto-1f389.json`,
67875 bytes, SHA256
`EAB1FD7070BFFD2965C12A80D2D91CFF76683AF65019F142ACF9A553EE877B5C`.
It has 1024x1024, 60fps, 92 frames and four adjacent alpha pairs (authored ind
2->3,4->5,6->7,8->9). Each paired layer has exactly one painted path, opacity100
and normal blend; provider-parent-to-content cases occur. Pair2->3 contributes
only on [20,31); the other three on [20,82). Artwork remains local-evaluation-only
under the existing source/license record; never commit it or silently normalize it.

Two additional blockers are explicitly part of original-input admission:
32 neutral `l:2` entries on layer `ks.a/p/s` vector properties, and bottommost
root `popper` authored `bm:10` (Difference). Pinned Telegram ignores unknown
blend codes; its output is NOT proof of general Difference support.

## Bounded admission and representation

Admit only root-composition, adjacent shape-layer pairs: exact integer `td:1`
provider immediately precedes exact integer `tt:1` content in authored order.
Each participating layer must produce exactly one painted draw (solid fill,
existing static linear fill or admitted solid stroke). Existing group transforms,
animated geometry, transform parenting and finite fractional opacity remain
allowed. Restricting pair cardinality avoids changing existing multi-paint
layer-opacity semantics; unpaired layers keep their current admitted scope.
Reject orphan/ambiguous/chained/self/nested/precomp/null pairs, duplicate roles,
other matte modes, masks, wrong types/values, and participating multi-draw layers.
At most64 pairs follows the existing128-expanded-layer budget; no new larger caps.

Add `matteSource` and `matteTarget` typed SourceNodeIds to own layer bindings,
lower them to stable layer indices in OwnSceneLayer, and publish
`EvaluatedLayer::matteSourceLayer` / `matteTargetLayer` (invalid by default).
Only content has `MatteMode::Alpha`; reciprocal provider identity makes source
suppression explicit. Record SourceNodeRecord `matteSource` and target Alpha
metadata. Pairing never alters `parent` or `transformParent`.
Both lowered model layers carry `StaticDependencyMatte`. Preserve relationship
identity even with inactive/empty draw ranges. Model, scene and plan topology
fingerprints include nondefault relationship identities; default-only historical
schemas remain byte-identical. Validators reject invalid or nonreciprocal pairs.

Admit property `l` only as exact integer2 on Layer-owned Vec2 Anchor/Position/Scale
properties; it changes no values, tracks, flags or clocks. Wrong type/value or
other property/group contexts remain contextual failures.

Preserve authored Difference by appending SourceBlendMode::Difference (retain
existing enum values), and carrying `isolatedBottomDifference` in own binding,
program and scene. Admit only the absolute last authored root layer, shape,
unpaired, with `bm:10`. No nested/precomp or backdrop-below cases. Preserve the
source metadata, do not label it Normal. Difference over a transparent isolated
frame is source-equivalent at the FULL layer boundary, not against the host's
background. This limited equivalence is independently witnessed; it is not
general blend-mode support. [W3C compositing formula and Difference](https://www.w3.org/TR/compositing-1/#blendingdifference)

## Planner and fixed private execution

Mark BOTH roles with RenderFeatureMatte and isolated Difference with a new
RenderFeatureBlend bit. Public planner unsupported accounting and public draw
continue skipping these features; provider artwork must never leak. Populate
plan `presentationOpacity` and `presentationVisible` with the supplied state.
For provider items only, effectiveOpacity omits the common presentation opacity;
content retains it. Thus common opacity is applied once, not squared. Geometry
and presentation transforms retain their existing meaning and canonical ownership.

Add private `AlphaMatteBackendAccess::draw(Backend&, const MotionRenderPlan&,
const NativeContext&) -> DrawResult`, with fixed solid/static-linear/normal-alpha/
isolated-bottom-Difference scope. Whole-plan preflight validates every source,
canonical resource, finite payload, role/adjacency/root/one-draw relationship,
supported bits, reciprocal indices, ranges and complete ordered eligible item
list before any native draws/resources. Eligibility follows presentation
visibility/opacity, layer visibility and inherited opacity (provider excludes
common opacity). No partial plan, corrupted valid prefix, orphan matte, actual
clip/mask/dash or other gradient/blend may slip through. Default public/private
linear paths retain their earlier behavior.

Reuse existing primitive/geometry/paint/stroke dispatch and caches. Record a
transparent whole-frame command list; record each active pair's content and
provider separately, close them, and use the existing-device Composite effect
with input0=content,input1=provider, DESTINATION_IN. Replay pair results at their
content stacking positions and suppress provider standalone output. Finally
replay the closed whole-frame image source-over once to the saved host target.
This isolation establishes the bottom-Difference equivalence even over an opaque
host. Do not treat geometry clips as alpha masks or read masks back to CPU.
[Composite input/formula](https://learn.microsoft.com/en-us/windows/win32/direct2d/composite)

Save/restore original target and transform on every success/error path. Do not
own BeginDraw/EndDraw or host DPI, clipping, antialiasing, presentation and
device-generation lifecycle. Restore the target before closing captures; check
Close/CreateCommandList/CreateEffect/SetValue results, discard failed captures
and propagate native/device-loss errors. Replay at identity after geometry has
already captured the host/presentation transform; test this against native pixels.
[Target changes](https://learn.microsoft.com/en-us/windows/win32/api/d2d1_1/nf-d2d1_1-id2d1devicecontext-settarget),
[command-list errors](https://learn.microsoft.com/en-us/windows/win32/api/d2d1_1/nf-d2d1_1-id2d1commandlist-close)

Use transient per-draw lists/effects initially; do not add mutable effect caches
without measurements. `drawnItems` counts source geometry issued into recordings,
including consumed providers, not standalone host contributions. An inactive or
empty input gives zero pair contribution; no unmasked content. Retain honest
draw/skip/error counts. Existing render-item budgets bound work.

## Evidence and delivery

TDD functional RED precedes each production behavior. Owned fixtures prove
pair admission/rejection, nonreciprocal corruption, stacking, fractional alpha,
presentation alpha once, provider RGB independence, inactive boundaries,
transform parenting, reverse/repeat, aspect fit and canonical resource lifetime.
Native API/command-list state and error injection are mandatory; fake contracts
are not raster proof. Literal interior pixels (per-channel allowance<=3) and
fresh pinned CPU captures keep the existing image comparison policy unchanged.
Do not graft own fields into an ordinary oracle that lacks pairing/local data.

Original Party gate requires unchanged hash, all92 frames in forward/reverse/
repeat and at least square+rectangular viewports; raster evidence covers0,19,20,
25,30,31,50,81,82,91. Include both original Ducks as regressions. Raw ordinary
scene export, independent CPU pixels and literal pair identities have different
coverage: label each, never call missing fields full-field parity.

Fresh full MSVC no-reference, Telegram and explicit windows-msvc-win32-preview
gates, independent task reviews and strongest whole-stage review precede handoff.
H then uses only the narrow private entry in existing OwnMotionRenderer. Preserve
strict unsupported-content handling, worker/transport/transparent canvas and
1/4/16 tests. Add a local-only pinned Party catalog entry after correctness;
no artwork in Git/Release. Stage a fresh `/MT`, variant-none Runtime candidate,
verify provenance/imports/symbols/tests, back up current Release+matching PDB,
and promote only with no running installed instance. Installed original playback,
pause/seek/restart and1/4/16 are separate UI acceptance gates. Report any unavailable
manual/DPI/GPU observation honestly. No speedup claims before matched Release measurements.
