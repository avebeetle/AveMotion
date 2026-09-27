# Part26K — own animation visible in Avelabs

## Intent and approval

Continue from AveMotion 1bab64b and Avelabs 712d454. The user wants animated
stickers inside the future Avelabs/AveVoice EXE, minimal machinery and ideas
checked against Telegram. This stage makes the already implemented own ellipse
path visible in the existing Motion Lab, not a new animation core or UI shell.
Controller approves this written architectural design under the user's explicit
delegation of ordinary reversible spec/plan/execution decisions. This does not
claim direct user review of this file. No automation is resumed.

Success is a separately built, statically linked Avelabs pilot using no rlottie:
own JSON/TGS preparation, existing Player and one worker/timer, own scene/plan,
Direct2D/WARP readback, owned QImage in the current canvas. Unsupported input is
reported explicitly; no silent reference fallback. The admitted subset remains
one supported ellipse/fill with existing position animation, not arbitrary SVG,
full Lottie or general Internet sticker compatibility.

## Alternatives and selected boundary

Selected: implement the small host-owned preparation/render adapter in Avelabs
src/app/motionlab, consuming current build-tree-private AveMotion headers. The
engine already has every parsing/model/playback/plan/draw component. There is
no reason to create a new installed SDK API, engine library, bridge registry or
duplicate scheduler for this pilot. Private includes stay PRIVATE to lab targets.

Direct native QWidget/HWND presentation would remove readback but adds window
composition, DPI and device-loss ownership changes. Defer it. Copying an ellipse
renderer into QPainter would bypass the engine under test. Reject it. A separate
generic host interface is unnecessary for two compile-time experimental routes.

## Build selection and compatibility

AVELABS_ENABLE_MOTION_LAB stays OFF by default. Add the cache string
AVELABS_MOTION_LAB_RENDERER with allowed values reference-cpu (default) and
own-warp. The existing reference route still selects telegram and Direct2D OFF.
The own route selects none and Direct2D ON. Reject unknown values, conflicting
explicit AVEMOTION_RLOTTIE_VARIANT, and own-warp without the lab enabled at the
root. No runtime backend selector, automatic fallback or public policy change.

Both experiments require no-install/fresh build-tree protection. Preserve the
reference-linked install prohibition, cache sentinels, static CRT selection and
normal OFF build. The own app must not build/link rlottie or avemotion_reference.
Runtime remains a linked first-party library because it houses own model/control;
the own worker must not instantiate Runtime or call its reference loaders.

Use new build/cmake/part26k-* and build/checks/part26k-* directories in Avelabs.
Never run build_ui.bat static, install, recreate Avelabs/out or overwrite accepted
build/Release (EXE SHA256 C92F26EE8FEB4FF03F6DC7F4AFFF4B11FB48142743D1EDCCB4E213AAA81F82C2).
Static app: Windows x64 Qt 6.10.0 Release /MT; standalone QtTest may use existing
dynamic Qt /MD and must be reported separately. No dependency installation.

## Concrete adapter

OwnMotionRenderer.h/.cpp are private Avelabs files. A tiny preparation function
accepts QByteArray and returns shared_ptr<const OwnNativeEllipsePreparedAsset>
or a descriptive QString error. Use detectAssetFormat/decodeTgs (default strict
sticker limits), readOwnJson, buildOwnNativeEllipseModel and
prepareOwnNativeEllipseAsset. Do not duplicate JSON or TGS decoding. Preserve
the own reader's stricter 1 MiB input, 4096-value, depth-32 limits; host file cap
remains 2 MiB. Report the rejecting stage/code/path and preserve the old session
on failed replacement. Existing width<=4096, frames<=18000, finite positive
rate/duration host guards also apply to prepared own metadata.

OwnMotionRenderer owns a single-thread WARP D3D11/D2D device, one reusable BGRA8
premultiplied target and CPU staging texture, one own-only planner and backend.
render(playback, tickTime, physicalSize, error) returns an owning QImage or null.
Evaluate with the SAME time sampled for Player::tick. Use physical viewport at
96 DPI; the canvas's existing DPR computation is unchanged. Render at most
1024x1024 per image and retain the worker's 4,194,304-pixel aggregate bound.
Clear transparent each frame, check full supported draw coverage and every
HRESULT, copy each mapped row using RowPitch into QImage-owned storage, and
always Unmap. Previously published QImages must remain immutable after later
renders, resize, reset and worker destruction.

Reuse target/staging for equal sizes. Resize rebuilds surfaces, not device.
reset() clears planner and backend resources on successful load/count replacement
before the retired own producers can accumulate cache entries; it need not
recreate the device. Graphics identity is private to this renderer/backend.
Device/target/draw/readback errors return descriptive stage/HRESULT, clear broken
resources, and stop the current automatic playback attempts in the worker.
An explicit subsequent load/play/seek may retry. No busy retry loop or fallback.
Once completion still commits on successful scene emission, not pixel readback;
do not change the engine's established playback contract.

Expose only actual lightweight renderer diagnostics needed by the lab/tests:
device creations, target rebuilds and successful readbacks. No allocation-free,
hardware GPU, latency or speedup claim follows from these counters.

## Existing worker and UI

Use compile-time own bindings at the current load/install/render sites. Keep
one MotionController, MotionWorker, QThread, single-shot QTimer, Player,
generation checks and bounded mailboxes. No alternate worker/control pipeline.
Own entries use addOwnNativeEllipsePlayback and typed lookup; Runtime fields of
ScheduledFrame remain null/invalid. Player controls, Freeze, user pause, count
1/4/16, resize, stale-load cancellation and shutdown behavior remain intact.
Replacement is staged; only successful preparation/registrations replace the
active asset/player/cache state. Copy needed tick records before Player mutation.

Own badge: "Experimental — own ellipse / WARP readback". Preserve reference
badge verbatim. Label own frame counters as readbacks, not reference CPU frames
or GPU presentation. Reuse Open/Play/Pause/Stop/seek/count controls and Voices
page; do not change docking, tray/Exit, window animation, settings or DPI logic.

## Evidence and source checkpoint

J already recorded Telegram's pinned coalesced work/weak-owner notification
pattern, applicable to the existing worker; no repeat framework investigation:
https://github.com/desktop-app/lib_lottie/blob/7d00b5048aff8dd93c0a9721abf50006d20682be/lottie/details/lottie_frame_renderer.cpp#L140-L172
Here reuse the existing coalescing, not Telegram source code or dependencies.
Read Qt QImage buffer-lifetime/format contracts and Windows Map/RowPitch contract:
https://doc.qt.io/qt-6.10/qimage.html
https://learn.microsoft.com/en-us/windows/win32/api/d3d11/nf-d3d11-id3d11devicecontext-map
Own application code is first-party; no vendor/licensing change.

Use the committed telegram_sticker_basic.json unchanged. Tests may generate a
temporary gzip/TGS from those exact first-party bytes, with provenance recorded;
the existing repeater TGS is a deliberate unsupported-own negative case. Do not
change corpus hashes or goldens, fetch stickers, or silently extend admission.

TDD requires compiling functional RED before implementation. Cover actual own
colored/transparent pixels and seek movement, row layout and image lifetime,
limits/unsupported/replacement, cache/surface reuse, multiple instances, hidden
Freeze/user pause, resize, generation cancellation, shutdown and page controls.
Reference tests stay valid under their original mode; any different own input
limit is tested explicitly rather than skipped. Run both complete Motion Lab
CTest variants sequentially, then static own app/own smoke, real build-boundary
guards/link graph and static import audit. Capture the own page via Qt grab for
visual inspection; it is not a desktop/native DPI acceptance claim.

Independent task review and one whole-stage review, with one combined final fix
and scoped re-review if needed. Preserve raw logs/SDD. Final engine none and
explicit windows-msvc-win32-preview tests retain inherited coverage; engine
sources are not modified here. Report actual observations, commands, both SHAs,
accepted Release hash and limitations. No artificial timed work or speedup claim.

## Self-review

Single visible vertical slice; no new primitive, scheduler or public loader.
Readback is intentionally an experimental output boundary. The accepted UI
package and reference mode remain intact. Full sticker coverage and direct
native presentation require separate measured follow-on decisions.
