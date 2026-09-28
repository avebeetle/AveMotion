# Part26N — unchanged Duck through own AveMotion

Status: source integration, whole-stage review and its single operational fix
accepted. A fresh static own candidate is installed and verified in canonical
`build/Release`. Native visual/controls acceptance remains pending: window
capture/input tooling failed after launch. This is a verified source/static
delivery, not completed end-to-end native acceptance. Do not repeat source tasks.

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
| `9330d7149309366d35b5e82c99f61eb849a32f51` | Minimal immutable playback metadata for host fit, selected-target capture test and final engine gates. |

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
Dynamic /MD smoke success is separate from the fresh /MT acceptance below.

## Frozen static delivery

Frozen build sources: engine `9330d7149309366d35b5e82c99f61eb849a32f51`,
host `f13099db64ab531881df6c836c19258860663324`; clean and equal to their remotes
before build. Both source commits ordinary pushed. Later documentation-only
commits are not compilation identity.

Independent whole-stage review covered57engine/13host files and local release
instruments. I1 found that a successful static audit could come from a different
tree. One final operational fix wave binds audit to exact tree,source pair,
app/smoke/PDB/stage hashes and compile/link/cache/project/install evidence,
revalidated before any canonical copy. Inert RED proved old mismatches accepted.
GREEN17/17,existing Duck proof8/8,recovery2/2; root fresh17/17 passed.
One scoped independent re-review accepted I1 with no new Critical/Important.
M1 generic primitive rejection diagnostics is deferred explicitly under ruling20.
These ignored-instrument changes did not alter the frozen product sources.

Fresh candidate tree: host `build/cmake/part26n-own-release-001`.
Evidence: host `build/checks/part26n-promotion-001`.
Build324.420s,exit0;staticCTest1/1,0.13s;explicit original Duck smoke exit0,
512x512,frames0/mid,2readbacks,1target. Actual graph/link/no-reference audit passed;
11actual compiler records/119commands all Release /MT. Compiler identity probes
excluded explicitly. No engine SDK install rules. Runtime staging/readiness,
ordinary/delay imports,exact26files,24unchanged notices and matching PDB passed.
All tracked input hashes stayed stable; prior package/PDB fully backed up.
This does not establish runtime LoadLibrary behavior,complete SBOM or licensing.

Actual root commands (existing pwsh runtime, repository root):

```powershell
pwsh -NoProfile -File out/part26n/build-candidate.ps1 -HostHead f13099db64ab531881df6c836c19258860663324 -EngineHead 9330d7149309366d35b5e82c99f61eb849a32f51 -Attempt 001 -DuckTgsPath C:/Users/USER/Desktop/AveMotion-CorpusLab-Part24/out/real-stickers-2026-09-28/assets/a-3.tgs
pwsh -NoProfile -File out/part26n/audit-candidate.ps1 -Tree D:/rvc/c++/DragonianVoice/Avelabs-UI/build/cmake/part26n-own-release-001 -Evidence D:/rvc/c++/DragonianVoice/Avelabs-UI/build/checks/part26n-promotion-001
pwsh -NoProfile -File out/part26n/promote-candidate.ps1 -Evidence D:/rvc/c++/DragonianVoice/Avelabs-UI/build/checks/part26n-promotion-001
```

Each exited0;logs `out/part26n/task4/005-root-candidate.log`,
`006-root-static-audit.log`,`007-root-promotion.log`. Do not rerun001 or overwrite
evidence. Exact CMake/CTest/CLI parameters and timestamps are in host command JSON.
Actual bound audit ran after fix acceptance.

Canonical promotion completed `2026-09-28T01:09:20.7980153Z`, with source/hash/
process guards and installed readiness/symbols checks passed. Installed identity:

- EXE SHA256 `A0EF0D9BE4642F38E466615BD19FE0B8989B382FBA78B25B2D9745D0F3EE3BC0`,52,712,960bytes.
- PDB SHA256 `3F4DF26FD20A35922BC98DD6CB0F60C0EDC0C60D5755CB3947B5F0A0F90B8485`,156,430,336bytes.
- EXE/PDB GUID `16407327-5c3f-4d83-abef-edd8a280f72f`,age1.
- Smoke SHA256 `292B6562628CFD240952084884E84219F391E05B90EF06C631A816C888A0B18A`.
- Full previous package/PDB: host `build/checks/part26n-promotion-001/backup`.

Previous Part26L EXE SHA-256 (backup,not current):
`93B10E5C93A3182BC59A4333F5444E95F4D4CA32D506AA05E26D1F740C5C1645`.
Previous PDB SHA-256:
`014ACD8CE9B3AFAB44A80F1ECEEAAC766D606EC958DC08DC64A392767173DFFF`.

## Native attempt and remaining gate

After promotion, canonical PID32720 was launched2026-09-28T01:09:32.808134Z.
Read-only check reports responsive; EXE hash matches above. Computer Use selected
the actual returned window. Capture failed `FrameArrived timed out`; one fresh
selection/activation retry failed `window capture timed out`. Accessibility reads
expose Voices and correct own-vector/WARP controls,No file loaded,0readbacks.
Open click failed `coordinate input geometry is unavailable`; one Tab gave no
confirmed focus change. No file entered/loaded,no image captured,input stopped.
User asked about desktop availability; a locked desktop is not inferred as fact.
Owned process left running unloaded,not terminated or hidden as Exit.

Evidence: host `build/checks/part26n-native-001/REPORT.md`,identity.json,
accessibility-final.json. Pending: actual Duck visible frames,play/pause/seek/
loop/Stop,1/4/16tiles and normal tray Quit/process exit. Do not convert Qt harness
captures/static smoke into missing native proof. Resume only this gate on an
operable desktop,not completed source work. Mixed-DPI,multimonitor,native drag,
hardware GPU,quantitative speedup,zero-allocation/race-detector guarantees remain
unverified/outside scope. Ordinary app geometry/dialog/log persistence is allowed,
not settings-isolated. No manual Windows/settings/dependency change was made.

Whole-review declined judgments: actual static provenance now verified above;
native controls/Exit pending; broader Lottie semantics,hardware/performance/race
guarantees,artwork distribution and optional unavailable Samsung coverage are
outside this milestone,not silently credited. Samsung is not called all-green.
Next after native acceptance: inventory another unchanged provenance-recorded
sticker,select its smallest missing feature,test through this same UI. No new
DLL/UI/scheduler or general-engine rewrite is assigned by this handoff.

## Design decisions

All delegated rulings, reasons and costs are recorded in
`docs/superpowers/ledgers/2026-09-28-own-duck-playback.md`; the binding scope is
`docs/superpowers/specs/2026-09-28-own-duck-playback-design.md`. Current execution
checkpoint is `docs/superpowers/STATE.md`. No new automation, dependencies,
Windows changes, external artwork publication or licensing decision is part of N.
