# Own mixed geometry composition design

Status: approved for implementation under the user's delegated design and
execution authority on 2026-10-12. This is a new functionality stage after the
completed public API and bounded output-buffer work, not a continuation of
allocation optimization.

## Result

A supported animation can combine an ellipse, a rectangle and a contour in
direct sibling groups of root shape layers, through the existing public own
playback interface. Geometry, transparency, ordering, seeking and independent
instances must remain correct. The result is a general composition capability,
not a renderer for a named sticker.

## Existing route and choice

The current dispatcher sends any document containing `el` or `rc` to the legacy
primitive compiler, even when it also contains `sh`. The canonical vector model
and its scene lowerer currently admit only `sh`; the existing scene materializer
already knows how to generate primitive paths.

Select once by authored vocabulary: any `sh` chooses the vector compiler;
`el`/`rc` without `sh` keeps the legacy primitive compiler. Do not retry another
compiler or reference renderer on failure. Extend the canonical vector model
with the existing Ellipse and Rectangle source kinds and property semantics.
Reuse the evaluator, scene program, path generators, Player and Direct2D backend.
The public API and Qt-neutral scene output do not change.

Pinned Telegram rlottie groups use common path items and reverse authored
painting order. That is the relevant architectural reference, not a claim about
the latest Telegram engine or GPU use:
[lottieitem.cpp at 67f103b](https://github.com/TelegramMessenger/rlottie/blob/67f103bc8b625f2a4a9e94f1d8c7bd84c5a08d1d/src/lottie/lottieitem.cpp).
Reuse existing geometry implementation and preserve its recorded provenance;
do not copy additional upstream code or make a distribution license decision.

## Admission contract

New primitive groups contain exactly `[el|rc, fl, tr]`: one geometry, one local
solid fill and the existing supported group transform. Existing `sh` grammar
and supported path-only animations remain unchanged.

| Input | Required values |
| --- | --- |
| `el` | `p` components in `[-32768,32768]`; `s` components in `(0,16384]` |
| `rc` | Same `p` and `s`; `r` in `[0,16384]` |
| `d` | Exact integer `1` or `3` |

Use the current vector property grammar for the new properties: static values,
hold/linear/equal-channel temporal cubic tracks, 2–256 strictly increasing
keys with `s`. Unsupported `e`, expressions, spatial tangents and unequal
channel easing remain explicit errors. Vec2 retains the current vector reader's
two components or neutral third component; nonzero third components are errors.
The strict positive lower size bound must reach both Vec2 components, including
animated keys. Positive fractional sizes are valid. Roundness above half the
smaller side uses the existing generator's clamp. Static primitive geometry that
collapses during float materialization must fail preparation rather than publish
an empty canonical resource.

For this first mixed profile, primitive involvement in trim, stroke, gradient,
outer paint/trim, nested groups, precomposition or alpha matte is unsupported.
Reject those combinations atomically. Existing path-only precompositions,
matte/trim/gradient and legacy primitive-only behavior remain unchanged.

## Correctness and ownership

Preserve authored source roles and reverse group draw order. Static local
geometry remains asset-owned; animated `p`, `s` or `r` is instance-evaluated.
Transform/opacity animation alone must not reclassify static local geometry.
Equal geometry content keeps distinct source identities. Published scenes and
images cannot change when another frame is evaluated or an owner is destroyed.
Mixed assets use existing vector aspect fitting; legacy primitive-only assets
retain their existing fitting and keyframe vocabulary.

## Verification and handoff

Add an authored small mixed fixture and public regression tests before production
code. Test literal geometry/order, static resource classification, animation at
endpoints/midpoints/hold boundaries, repeated out-of-order seeks, independent
instances, retained scenes and explicit invalid-input errors. Keep legacy
primitive and existing Duck/Party regressions.

Use existing vector differential and Direct2D capture targets with the pinned
ordinary fresh reference. Compare both directions, fractional dimensions,
rectangle radius and asymmetric path content without relaxing pixel policies or
changing goldens. Run the reference-none suite, packaged public consumer,
`windows-msvc-win32-preview`, and Avelabs own regressions. Reuse the existing
Test Animation page for widget capture and control checks; no new viewer.

Use the saved measurement kit to check existing Duck/Party behavior and load,
not to require a speedup from this functionality change. Keep original baselines
and raw evidence. Only promote a checked static reference-none build to
Avelabs `build/Release` with the existing backup/provenance process.
Automated image and controlled-scale checks do not close native mixed-monitor
visual acceptance; the user currently has no PC access.

## Exclusions

No new scheduler, per-icon thread, large cache, resource manager, GPU route,
format-wide redesign, multi-path merge, public API redesign, docking/physics
changes, dependency installation, Windows changes, license change or artwork
publication. Preserve vendor sources/notices, existing measurement artifacts and
protected installed binaries until delivery gates pass.

## Design approval

Self-review: admission, animation vocabulary and legacy dispatch are explicit;
all new geometry uses the existing canonical pipeline. Tests cover the positive
fractional boundary and independent ownership, not allocation counters alone.
The deliberately limited combination profile is not full Lottie support.
