# Supported own playback and one Avelabs UI icon

The next result is a usable boundary for the existing own engine, demonstrated by
one real UI element. Preserve the accepted baseline first. This stage does not
add Lottie features, a scheduler, a large cache or a hardware rendering path.
The user delegated the detailed design, implementation choices and ordinary
scoped commits/pushes; this design records that approval within the requested scope.

## Starting point

Canonical repositories are `C:/Users/USER/Desktop/AveMotion` (E) and
`D:/rvc/c++/DragonianVoice/Avelabs-UI` (H), main at E `ba7cda9` and H `2ee237c`.
The accepted initial measurement uses unchanged production sources from installed
builds E `69ebbb1` and H `69fe01b`. H
`docs/testing/motion-baseline-2026-10-11/REPORT.md` defines its timing and display
limitations. Product sources must remain unchanged until the source instrument,
protocol and original measurement binary are frozen and hash-checked.

## Telegram boundary and decisions

The pinned [Lottie Icon header](https://github.com/desktop-app/lib_lottie/blob/fbb922c30b7c30ba17ad5fd4fcb989eadd620062/lottie/lottie_icon.h)
offers construction, frame/paint access and animation control without exposing
the composition compiler or rasterizer. Its
[implementation](https://github.com/desktop-app/lib_lottie/blob/fbb922c30b7c30ba17ad5fd4fcb989eadd620062/lottie/lottie_icon.cpp)
keeps current/preloaded images, uses shared asynchronous tasks and weak lifetime
guards, and returns prepared images to UI painting. The
[toast consumer](https://github.com/desktop-app/lib_lottie/blob/fbb922c30b7c30ba17ad5fd4fcb989eadd620062/lottie/lottie_toast_icon.cpp)
separates async readiness from guarded UI painting. Some accessors wait on a
semaphore: do not copy that blocking behavior into our paint callback. These are
architectural ideas, not copied implementation, assets or dependencies.

Use an additive opaque own-playback facade in the existing E Rendering library.
Putting the current preparation into Runtime would reverse the existing
Rendering -> Player -> Runtime dependency. Keep legacy Runtime loading and its
no-reference `ReferenceUnavailable` behavior intact; the new named own route
explicitly supports the already admitted own profile.

H will share one execution thread across independent existing worker sessions.
Each session retains its Player, timer, renderer, cancellation/generation state
and bounded mailboxes. This reuses the current scheduling and avoids a thread
per icon without rewriting registry/transport semantics. The smallest real UI
integration is an own-authored animated icon on the existing TTS navigation
button, currently text-only: play on hover/press, stop at rest, suspend when hidden.
Show its own initial image while stopped; do not add an endless decorative loop.
Text, navigation,
checked state, window/docking physics and the Test Animation page remain intact.

## Supported engine contract

Expose opaque immutable asset and mutable playback ownership through installed
public headers under `include/avemotion/render/`. A byte preparation function
detects JSON/TGS, uses existing strict decoder and own admission, owns its source
data, and returns owned structured errors. Preserve host limits: 1 MiB decoded
JSON, 65536 values, depth 32, canvas dimensions 1..4096, 2..18000 frames, finite
positive timing. Unsupported data is an explicit error, never reference fallback.
Do not expose authored JSON/model bindings, private scene programs or erased
Player operation tables.

Playback delegates controls, evaluation and successful completion publication to
the existing own implementation. Reuse existing Player registration/typed lookup;
Player retains the public playback owner, distinct registrations are independent,
and own scheduled frames keep null Runtime identity. No additional scheduler.
Asset/scene ownership remains valid after source buffers or playback are gone.
Access to playback, Player and drawing stays serial on the control/render thread.

Provide a narrowly documented target-size helper preserving current primitive
box behavior and integer vector aspect fitting, including odd/non-square boxes
and rejection when a positive fitted target cannot be made. Provide an additive
named fixed-profile Direct2D draw entry for already supported own gradients,
alpha mattes and blend modes; preserve complete preflight and the conservative
policy of the existing generic `Backend::draw`. Host owns device, target,
BeginDraw/EndDraw, readback and presentation. Core APIs do not depend on Qt/QImage.

An installed no-reference consumer must prepare, instantiate, control, schedule
and evaluate supported own data with only public headers/libraries. This is a
local supported boundary, not new stable-ABI or redistribution/licensing approval.

## Host and icon contract

Remove every engine-private include path/header from H production consumers and
public-path smoke/renderer tests. Keep host WARP creation, planner/backend reuse,
readback/copy, limits and failure/retry behavior unchanged.

Share the executor explicitly or through existing application ownership, with
queued initialization for sessions added after it starts. Destroy a session on
that thread after invalidating only its work; never quit the shared thread when
one icon/controller dies. Final service shutdown drains and joins once. UI
callbacks are guarded by session generation and QObject lifetime. No waiting for
preparation in paint. Independent sessions must not share transport/visibility.
Independence means state and lifetime isolation, not uninterrupted frame cadence:
bounded preparation/rendering on the common thread can delay another session.
Test its unchanged state and eventual progress and report observed delays.

The icon adapter is thin: use the existing controller/worker path, accept owned
premultiplied images, and update a normal button icon on the GUI thread. Use one
small project-authored JSON resource within the existing profile, not a downloaded
sticker or special parser. Logical icon size maps to actual physical pixels by
DPR. Transparent pixels compose with the existing button background. An idle,
paused or hidden icon does not continue rendering after pending work drains.

## Verification and comparison

Functional RED precedes implementation; GREEN includes public-only installed
consumer, invalid/unsupported input, source/asset/playback lifetime, controls,
independent instances and unchanged generic backend policy. H tests exercise two
independent sessions on the same executor: controls, replacement/error, visibility
and destruction of A cannot change B. Verify stale work is not delivered and an
owned image survives session destruction. Test icon interaction/transparency/DPR
and exactly one unchanged navigation request.

Rerun existing supported Duck think, Duck X3 and Party rendering/worker/page gates,
shell tests and own/no-reference/package boundaries. Graphics gate is E
`windows-msvc-win32-preview` including capture/WARP/device recreation. Preserve
existing goldens and distinct exact/qualified image parity contracts.

Compare frozen A and candidate B with the same minimal instrument, assets,
256x256 physical per-image raster, observed DPR, 1/4/16 and existing phases.
Run serially without builds/tests/other workers. Record source/binary/tool/input
hashes, raw output, CPU/memory, load/group/delivery/Qt-paint timing and settled
pause/hidden behavior. Distinguish repeated baseline variation from performance
claims. Add a bounded icon-active/rest/hidden observation using the existing
resource observer; do not invent a measurement framework or monitor/GPU fence.

## Exclusions and completion

No format expansion, compiler-dispatch rewrite, new scheduling algorithm, pool,
large cache, GPU redesign, reference fallback change, dependency installation,
Windows change, docking/physics change or history rewrite. Keep vendor provenance,
notices and reference-linked install prohibition. No foreign artwork in Git or
Release and no new licensing decision. Work in existing main by user authorization.

Completion means a reviewed public own route, one real icon in the existing UI,
independence/regression evidence, repeatable before/after data and a guarded
Release handoff with honest limits. A measurement regression must be investigated,
not concealed or automatically optimized within this stage.
