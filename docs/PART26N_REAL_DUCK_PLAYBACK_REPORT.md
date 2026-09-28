# Part26N — unchanged Duck through own AveMotion

Status: engine Tasks1–3 accepted and pushed; Task4 host integration independently
accepted. Whole-stage review, frozen static candidate and canonical native
acceptance are still in progress. This document
does not yet certify a new `build/Release` installation.

## Target and scope

Play the unchanged official Telegram Duck think TGS through AveMotion's own
compiler/evaluator/scene stream, then the existing Avelabs Player/worker/WARP
renderer and Qt canvas. No reference/raster fallback, second scheduler, new UI
shell or DLL. Canonical destination is
`D:/rvc/c++/DragonianVoice/Avelabs-UI/build/Release/AvelabsUI.exe`.

The local-only input is `out/real-stickers-2026-09-28/assets/a-3.tgs`, from
TelegramMessenger/TelegramStickersImport commit
`b8951c8a02b245142142d341644d0756cf16278c`, `Example/Source/a-3.tgs`.
SHA-256: `7CD55718288EB1B1D1EDFD0E774D9038E076A7F5AADF9835A92CF942948A4BC1`.
It is 4,635 compressed bytes / 60,315 JSON bytes, 512×512, 60fps, 180frames.
Artwork is not committed, bundled or newly licensed for redistribution.

## Accepted engine work

| Commit | Result |
| --- | --- |
| `8b18cfe3c1a9a4a04ae63ebd8ba40d6b63caf571` | Accepted carried multi-resource model/scene prerequisite, including correct canonical resource keys. |
| `53cbad79f5782bf113cd80ae01b3b00ad17d0efd` | Bounded own vector compiler with exact pre-conversion numeric admission. |
| `d7aa637b12d772d534c2dc932c035f86a815f540` | One shared scene program/emitter, pure path materialization, correct final-space stroke resources and conservative clip-equivalence guard. |

Functional RED precedes implementation and review fixes. Independent task review
and scoped fix review accepted each task. No vendor or golden edits; explicit
real-input invocations fail when the supplied file is absent.

Final Task3 gates on frozen reviewed sources:

| Check | Result |
| --- | --- |
| Full no-reference MSVC | 47/47, 9.20s |
| Full Telegram MSVC | 100/100, 124.25s |
| Explicit `windows-msvc-win32-preview` | 96/96, 141.05s |
| Original Duck ordered scene comparison | 384 samples; 36 draws; topology/order/paint checks pass |
| Original Duck same-backend WARP pixels | 27/27 exact, 0 skipped draws; independent CPU policy 27/27 |
| First-party capture matrix | 90/90 exact WARP, CPU policy 90/90 |
| Root fresh focused check | 4/4, 3.46s; separate original lifecycle, 384-scene and 27-capture commands also passed |

These are test durations, not a performance comparison or speedup claim. WARP is
software rendering, not hardware GPU evidence. Sequential1/4/16 stream checks
are not a race-detector result.

Full visible scene maxima: final path/bounds `3.05176e-05`, world matrices
`2.28882e-05`. Pinned-reference local export applies float `M*inverse(M)`;
raw local maximum remains `0.0001220703125`, with 3,075 over-limit fields. A
structurally constrained test-only forward representation check has residual0;
raw error is **not** called zero or within1e-4. Original unmodified scenes drive
the pixel tests. Exact inactive-miter handling covers13 roles; active fields and
mutation tests remain strict. See design ruling13 for the complete contract.

Engine evidence is retained under `out/part26n/task1`, `task2`, `task3` and the
matching ignored SDD workspace. Final Task3 logs069–085 preserve commands/results;
raw earlier failures are retained. Final author captures are
`out/part26n/task3/captures/run-38328552604500` (real Duck) and
`run-38290043701000` (first-party). Inherited rlottie header C4251 warnings remain
visible in reference-linked gates; the stage is not described as warning-free.

## Viewport decision and limitations

Independent review found that omitting a precomp canvas clip is wrong when it
can affect letterbox pixels. The engine now proves conservative clip equivalence
or returns `UnsupportedClipping` atomically, preserving successful history.
First-party wide/tall fill and pen spill witnesses protect this boundary;
unrelated root-layer drawings and primitive viewport behavior are preserved.

The deliberately conservative miter envelope rejects actual Duck's direct
256×512 portrait request at frame0. This is a disclosed false rejection, not
evidence that its raster really spills. We did not add a new stroker/tangent
subsystem to circumvent the guard. Task4 instead uses a deterministic vector
composition-aspect target within the host's existing bounded request, with
centering performed by the existing canvas. Primitive/reference sizing stays
unchanged. Actual raster size must be distinguished from the bounding request.

This host policy presents the intrinsic vector canvas, so root artwork outside
that canvas is cropped. Rounded non-square fits still face the engine guard;
unrepresentable sizes or clips remain explicit errors. Widget resampling is not
claimed pixel-identical to a reference rendered at a different target size.
General gradients, masks/mattes, nested/transformed precomps, images, text,
repeaters, dashes and broader Lottie semantics are outside this bounded profile.

## Host and delivery checkpoint

Host base: `0d56e83214c0e64167a717c53b337e355e6595dd`.
Final Task4 Qt gates: reference3/3; own opt-in4/4 and own working-entry4/4,
with explicit original Duck inputs and0 skipped Qt cases. Own Qt totals each
33worker/13page/12renderer passes; reference31worker/11page. These include
init/cleanup slots, not that many independent user scenarios. Python deployment97,
diagnostics34, build29 and runner3 passed. Review I1 stale visible raster, I2 Duck
transport/visibility and I3 same-actual-target pixels were addressed in fix1 and
independently accepted. Root fresh fix checks each3/3 pass. Actual Duck Qt page
captures show distinct frames; they are not canonical-native evidence. Test
output-root collisions were corrected without overwriting previous L evidence.

Final root full MSVC gates: none47/47 (7.91s), Telegram100/100 (127.18s), explicit
preview96/96 (129.76s), logs out/part26n/task4/002–004. Final external capture adds
129 to the earlier matrix:36/36 exact own/reference WARP cases, CPU policy36/36.
Host QImage pixels from both257x129 and129x257 bounding boxes exactly match the
ordinary-reference129x129/frame0 BGRA (66,564 bytes, SHA256
`7D7482DD199FA7D70B93EC06CD88686BE0F6C14A05760933F737A6FAB8D03466`). Missing,
truncated and wrong-frame reference inputs fail. Evidence:
out/part26n/task3/captures/run-41132590301500 and host build/checks/part26n-review-*.
Dynamic /MD smoke success is not substituted for the pending frozen /MT candidate.
Whole-stage independent review and final frozen source identities: pending.
Static candidate/link/import/notices/PDB audit: pending.
Recoverable canonical promotion and native load/play/pause/seek/count/Exit: pending.

Protected installed Part26L EXE SHA-256:
`93B10E5C93A3182BC59A4333F5444E95F4D4CA32D506AA05E26D1F740C5C1645`.
Protected PDB SHA-256:
`014ACD8CE9B3AFAB44A80F1ECEEAAC766D606EC958DC08DC64A392767173DFFF`.
Those hashes were freshly checked after Task3; no promotion has occurred.

Native testing, when performed, uses the actual owned canonical process and its
ordinary geometry/file-dialog/log persistence. It is not settings-isolated and
does not imply mixed-DPI or multimonitor acceptance. Qt test-page captures alone
do not certify the canonical executable.

## Design decisions

All delegated rulings, reasons and costs are recorded in
`docs/superpowers/ledgers/2026-09-28-own-duck-playback.md`; the binding scope is
`docs/superpowers/specs/2026-09-28-own-duck-playback-design.md`. Current execution
checkpoint is `docs/superpowers/STATE.md`. No new automation, dependencies,
Windows changes, external artwork publication or licensing decision is part of N.
