# Part26H — own ellipse scene stream

## Intent and approval

Build the next constrained piece of AveMotion's own Lottie/TGS path: emit owned
scenes and usable render plans from the sealed Part26G prepared ellipse model,
without rlottie in that production dependency chain. The eventual destination
is static integration in the Avelabs-UI executable. This stage is not that host
integration, a general SVG/Lottie implementation, scheduling or production loading.

Controller approved this written architectural design on 2026-09-27 under the
user's explicit delegation of spec/plan and reversible technical decisions.
This does not assert that the user reviewed an unseen document. Base2606632.
Source/design inventory: out/part26h-design/identity-cache-audit.md. The old
identity-inventory.md resource stubs predate completed G and are not new tasks.

## Alternatives and selected boundary

1. **Selected: separate Rendering-private own stream.** Consume the sealed own
   prepared owner, reuse first-party evaluation/ellipse/path/model helpers, and
   assemble the small own scene schema directly. Old certificate-backed stream
   stays unchanged. A little scene assembly/history orchestration is repeated;
   numeric, easing, path and hashing algorithms are not copied.
2. A common generic scene-emitter descriptor could remove that orchestration but
   would introduce a new mutable metadata boundary and refactor a proven legacy
   path before there is a second general primitive. Defer it.
3. Public Runtime loader/identity replacement or a general sampler/MotionService
   would solve a larger integration problem but also change scheduling, policy
   and supported-input promises. Those require a subsequent design.

## Global constraints

- Direct main, scoped ordinary commits/pushes; root owns ALL Git writes. No force-push, rebase, amend or history rewrite. One product writer; retain raw/SDD evidence.
- No vendor, dependency, license/notice, fixture or golden changes; no dependency installation or Windows/settings/security/automation actions.
- No UI writes/builds/GUI or UI/out recreation; preserve Avelabs712d454 and accepted Release EXE C92F26EE8FEB4FF03F6DC7F4AFFF4B11FB48142743D1EDCCB4E213AAA81F82C2.
- No public loader, Runtime handles, Player threading, Direct2D ownership, ANGLE/backend coverage or public fallback changes. New production interfaces are private/not installed; reference-linked install prohibition stays.
- No changes to legacy NativeEllipseStream, NativeEllipseCertificate, own reader/admission/model policy or existing oracle/goldens to make parity pass.
- No full Lottie, own playback/GPU, universal scalar parity, TSan, zero-allocation or measured-speedup claim. Samsung is configure/provenance-only in this stage.

## Production contract

`render::detail::OwnNativeEllipseStream::create(shared_ptr<const
OwnNativeEllipsePreparedAsset>)` returns a typed result owning a unique stream.
Only the sealed G owner is accepted: no arbitrary graph, certificate, Asset or
caller-selected instance ID. Reject null/incomplete preparation and source hash0
without substituting fake identity. Existing authored/model ownership is retained.

One linked implementation allocates monotonically increasing nonzero uint64
stream identities, concurrency-safe and never recycled. Counter exhaustion is a
typed failure; neither identities nor emission attempt sequences wrap. Small
private counter functions used by production are directly tested at the last
valid value, not via setters on live streams. Failed construction may consume an
ID; no guarantee of contiguous successful IDs. Relaxed atomics suffice for unique
numbers, not object publication or shared-stream synchronization.

`emit(size_t frame, size_t width, size_t height)` produces an owned optional scene
or a typed error. Advance the attempt sequence before viewport validation; reject
zero dimensions, clamp frame to last frame, evaluate the prepared immutable model
with a per-stream evaluator/workspace, and apply existing aspect-fit translation.
Sequence exhaustion returns an error without wrapping or changing history. The
API takes integer frames, not elapsed seconds. A stream is single-writer;
different streams may be used concurrently with a shared immutable prepared owner.

Use the own binding and model's two-layer/one-node schema: root0, shape1, draw0,
geometry0, paint0. Root is visible; the shape is visible only in the half-open
authored layer interval. An inactive shape emits no draw but retains the frozen
model and resource counts. Literal own model layer names are display labels,
not Telegram escaped key paths. Keep default mask/matte/effect/stroke/gradient/
image fields. Resolve evaluated position/size with existing storage rules,
generate the ellipse through the existing generator and materializer, and apply
the frozen model with the existing model application helper. Animated tiny paths
may collapse exactly as the existing primitive does; preparation policy is not
silently tightened. Static collapse remains G's typed preparation rejection.

Scenes carry the exact source hash, invalid AssetHandle and InstanceHandle, the
factory ID, clamped frame, viewport and monotonically increasing attempt sequence.
Recompute normal scene fingerprints; only a successful emission updates previous
fingerprints/change flags. Failure leaves retained snapshots and successful
history unchanged. Each scene owns its vectors/model; static canonical aliases
retain the model control block. A render plan retains its scene and must remain
usable after all external stream/prepared/authored owners are released.

## Identity and cache domains

H requires an **own-only planner/backend domain** fed by one linked allocator
implementation. Several own sources/streams can share that domain. Do not mix
reference Runtime, legacy caller-ID streams or separately loaded allocator copies
in it: planner keys are untagged packed handles/raw IDs and can collide. This is
an explicit precondition, not a new runtime-enforced namespace guarantee.

The allocator implementation must outlive associated scenes/plans/history/cache
domains; destroy domains before module unload/reload. IDs never recycle, but
planner and animated backend cache entries still require caller forgetting/reset
to bound memory. Stream destruction does not mutate an externally owned planner.
Static geometry/paint can share source-based asset cache keys; animated geometry
keys include the own stream ID. Existing FNV hashes are not security identities.
No planner/backend schema changes or cache eviction service in H.

## Proof and comparison policy

### Literal no-reference and lifecycle tests

Compile production with reference=none. Pin independently expected layer/default
fields, transforms, paths, colors, source IDs, class counts, model ownership,
fingerprints and change behavior. Exercise JSON and existing TGS reader-to-stream
chains, visibility boundaries, static/animated values, invalid viewport, reverse/
repeated/mixed seeks, clamp, source release and source isolation. Preserve earlier
snapshots. Counter tests cover exhaustion. Concurrent distinct streams test
functional isolation/unique allocation, not absence of all possible races.

Use ONE planner to interleave same-source animated A/B, repeated frames and seeks:
both first plans are first; no cross-stream stale/history leakage; animated keys
differ and static paint keys match. Pin same-source static sharing, different-
source partitioning, stale/equal sequence, forget/rebuild and retained-plan life.
Synthetic copied-scene tests characterize duplicate-ID and packed-handle hazards
without making the factory accept arbitrary IDs.

### Telegram semantic scene/plan parity

Use the unchanged ordinary fresh `NativeEllipseOracle`, independently parsed and
sampled; never derive expected values from the candidate. Use all15 existing
NativeEllipseTestAssets, four viewports (512x512,256x256,384x256,256x384), forward
0..60, reverse60..0, repeated/mixed access and viewport changes. Compare scenes
and plans, not just fingerprints or counts. Own calls must not increment live
reference counters; none builds are the stronger production independence proof.

Own and reference metadata conventions intentionally differ. Normalize TEST COPIES
only, independently on each side, with a fixed documented semantic role mapping:
source-layer/group/ellipse/fill and bound properties; asset/instance identity;
literal-vs-escaped layer display labels. Before mapping, validate each original
value against its own binding/schema; never copy candidate values into the oracle.
Full model graphs were independently covered by G: remove both model pointers in
the scene comparison copy but retain model-applied/resource counts, draw resource
classification, canonical content and aliases' actual values. Recompute copied
scene fingerprints after normalization. Check original fingerprint computation,
sequence and success-only change history separately. The unchanged exact scene
comparator checks all remaining fields. Actual original scenes feed planners.

Normalize plan stamp/cache identity fields and attached sourceScene copies; exclude
only raw identity-dependent aggregate plan fingerprints, checking each original
against the existing recorder where available. Retain exact item content hashes,
revisions, updates, transforms, bounds, dirty/presentation flags, counts, changes,
plan sequence and default fields. Use unchanged exact plan comparison. Mutation
witnesses must prove changed geometry, color, transform, visibility, revision,
default paint fields and bounds are rejected. Any additional excluded semantic
field requires a documented design decision, not an ad-hoc mismatch whitelist.

### Windows pixel/cache/lifetime proof

Mechanically share the existing test-only WarpCaptureSurface helper with the new
test; preserve existing capture behavior/thresholds. WARP is software, not proof
of hardware GPU performance. Render own and independently sampled ordinary
reference scenes through separate planner/backend domains to equal BGRA pixels.
Evaluate scene viewport in logical dimensions, render CPU reference at physical
pixel dimensions, and use the existing capture profiles' DPI configuration.
Compare own WARP versus independent rlottie CPU with the UNCHANGED existing
PixelComparisonPolicy, reporting actual metrics rather than claiming bit equality
between rasterizers. Explicitly handle empty output and require visible controls.

Cover static, animated, visibility, nonlinear easing, nonsquare and fractional
fixtures; frames0,10,19,20,30,60 at all five existing capture profiles. Include
own backend A/B/A, static sharing/animated separation, retained old plan replay
after newer frames, source destruction, forgetting and resource-domain recreation.
Preserve raw metrics/artifacts. No production Direct2D changes.

## Completion gates and exclusions

Functional compiling RED precedes production, then GREEN and independent task
spec/quality review. One whole-stage independent review; root fresh full MSVC
none, Telegram and windows-msvc-win32-preview configure/build/CTest; Samsung
configure/listing, unchanged all-vendor/TGS integrity. New stream's actual compiler
include trace and generated link/install graph checks prove private no-reference
closure. Bind final raw evidence to exact source HEAD and retain failed attempts.
Check host Git/accepted executable/read-only absence of UI/out before handoff.

Report completed bounded coverage and unsupported behavior explicitly. Scheduling,
mixed producer identity, host UI acceptance, other primitives, generalized loading
and performance tuning remain subsequent stages. Do not imply a finished engine.

## Self-review

No placeholders; production, test normalization and consumer isolation are distinct.
Dependencies reuse G without weakening its sealed ownership. Full UI/public loader
work is excluded. Controller approves proceeding to the written implementation
plan under delegated authority; no routine confirmation is pending.
