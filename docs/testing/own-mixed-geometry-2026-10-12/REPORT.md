# Own mixed geometry — 2026-10-12

Status: bounded implementation, regression, measurement and local delivery complete.
The checked static own-engine build is installed in Avelabs `build/Release`.
Native physical-monitor appearance and mixed-monitor DPI acceptance remain OPEN.

## Result and limits

Commit `e0949a2e81b8036e2b6d9e7f68b56758d1f66099` lets the existing public own
playback combine contour, ellipse and rectangle sibling groups. It reuses the
canonical model, evaluator, scene program and existing primitive generators.
New primitive groups are exactly `[el|rc, fl, tr]` in root shape layers; this is
not full Lottie support. Existing path-only and legacy primitive-only profiles
remain separate, with explicit unsupported-input errors and no compiler retry.
The public interface, Player, renderer ownership and host UI behavior are unchanged.

The first-party `tests/fixtures/own/mixed_geometry.json` contains three authored
groups and 61 frames. Public regressions cover literal geometry/order, positive
fractional sizes, primitive property animation, independent playback, retained
scenes and unsupported scopes. No external artwork or new upstream code was added.

## Completed checks

- Functional public/model/stream RED preceded production, then focused3/3 and
  full reference-none45/45 passed. Independent Task1 review: no findings.
- Fresh reference-none install package46/46, package-boundary check and separate
  installed public consumer configure/build/run passed. The local package has no
  reference renderer; this is not permission to redistribute derived code.
- Full `windows-msvc-win32-preview` build and CTest97/97 passed at the frozen
  compilation revisions below (VS2022/x64/Debug, Telegram oracle, capture/WARP
  and device recreation). These are not substitute non-graphics gates.
- Task2 full model and stream differential tests passed. Focused mixed capture
  produced455 byte-exact own/ordinary-WARP samples:453 CPU-policy passes and
  two explicitly guarded expected-negative CPU rejections below. Three
  missing/moved/recolored mutation witnesses are caught; no policy was relaxed.
- Separate Qt test build passed mixed-page load/Play/Pause/seek,1/4/16 images
  and resize, using independent133x100/100x75 expectations. Final controlled
  icon DPR1/1.5/2 runs each passed3 QtTest checks (including lifecycle), with
  physical20/30/40px output. The38 final PNGs show transparent composition and
  state changes; they are not physical-monitor or live-pacing acceptance.
- Fresh host own6/6, final reference4/4, shell3/3 and static2/2 passed.
  All nine explicit original Duck a-2/a-3 and Party renderer/worker/page slots
  passed with no skips, as did the three static original-asset smokes.
  Optional external skips in the ordinary own suite are retained (page1,
  worker2, renderer3); the separate original-asset runs supply that acceptance.
- Independent Task2 spec/quality review approved the test-only slice with no
  findings, independently checking all455 WARP pairs and literal CPU witnesses.
  Final whole-stage review approved protected promotion with no blocking findings;
  all20617 bound audit artifacts were independently checked. Installed audits and
  the exact26-file/PDB package were subsequently independently checked too.

The initial45-test run was44/45 because an unchanged child Ninja configure
could not find `cl`. Command-scoped VsDevCmd recovered it; original failure logs
were retained and no product/CMake workaround was added.

The first fresh host reference run was3/4, exit8, at the unchanged pause test
barrier in `tests/motionlab/tst_motion_page.cpp:737`. An unchanged focused
recheck passed3/3; two unchanged full rechecks passed4/4. All failed and passing
receipts remain. A pending frame notification may drain the final paused batch
before the queued control acknowledgement, so requiring another delivery after
that acknowledgement is not guaranteed. This interleaving is inferred from the
source and failure signature, not traced in the failed run. Independent reviews
classified it as a nonblocking P2 test reliability follow-up, not evidence of a
product pause regression. A deterministic barrier must later cover both the old
slot and the analogous mixed external-fixture slot at line1506, preserving the
pause/freeze assertions. No assertion, tolerance or production fix was made here.
Inherited C4100/C4505/C4251 warnings remain; the builds are not warning-free.

## Subpixel comparison boundary

At512x256/frame60, the0.5x0.25 ellipse has four CPU alpha values7,7,6,6 versus
own/ordinary-WARP9,9,11,11. The unchanged active threshold is alpha>8, so CPU
omits only this ellipse from the global active bounds: delta81 versus limit4.
The other five policy criteria pass; all524288 own/ordinary-WARP raw bytes match.
CPU compared with independent ordinary-WARP fails identically. Independent audit
confirmed this is a cross-raster threshold boundary, not an own geometry mismatch.

The approved design addendum requires a narrowly named expected-negative test:
assert the sole policy rejection, all other criteria, exact independent WARP
and literal nonzero/color/position witness. Keep `cpu_pass=0`, the original failed
receipts and all authored inputs/samples. Do not describe it as CPU-policy PASS.
No global tolerance, golden, rasterizer or asset change is permitted for this case.

## Evidence and environment

Engine raw receipts: `out/checks/2026-10-12-own-mixed-geometry-001`.
Host raw receipts: `D:/rvc/c++/DragonianVoice/Avelabs-UI/build/checks/2026-10-12-own-mixed-geometry-001`.
C disk exhaustion interrupted a later capture run; the actual failed LastTest
and unreliable Tee-masked shell0 are retained, not counted as success. Root moved
only this stage's7892 capture files (2125847079 bytes) to host `task2/engine-captures`,
with per-file size/SHA256 verification. Historical log paths are not rewritten;
`task2/capture-relocation-before.json` and `capture-relocation-after.json` bind the
original/destination inventory. Old stage evidence and installed binaries remain.

The relocation-after JSON has a null aggregate-byte field from a PowerShell
ordered-dictionary aggregation quirk. The per-file size/hash manifest and actual
destination establish the2125847079-byte inventory; that null is not a new total.

## Saved measurement comparison

The unchanged five-file `tools/diagnostics/motion-baseline` kit and existing
`observe_process.py` ran six serial Duck think/Party1/4/16 cases. Each image was
256x256 physical pixels at observed DPR1, with30s steady playback, pause, hide
and resume. Inactive measurements exclude1s settling and cover about9s each.
Windows10Pro10.0.19045, Ryzen5 5500/12logical CPUs; strict idle preflight found
no builders, tests or GUI. Authorized PIDs68424/15484/65860 had already exited
naturally; no process was terminated. Collection/analysis exit0, six probe exits0.
Observer exits3 mean the owned probe exited normally before the observation
duration ended. No degraded native samples occurred.

Historical A is `build/checks/baseline-2026-10-11-001/matrix-001/analysis.json`,
compiled E2a133b84a261a4652b6ac24ea2ff2c4ef78d7c20 /
Hb78e5325fb2d0c8447d214d7c771db4e036de6b4. It predates public integration, the
UI icon and buffer reuse. Current B is `build/checks/2026-10-12-own-mixed-geometry-baseline-001/matrix-001/analysis.json`,
compiled at this stage's frozen revisions. This is a cumulative historical
comparison, not attribution to mixed geometry. A did not record the original
tool-binary hashes; later preservation hashes cannot prove those original bytes.
The unchanged protocol intentionally retains A's identifiers; B's actual source,
build, asset, tool and launcher identities are in `frozen-inputs.json` and command
receipts. No statistical speedup or isolated regression is inferred.

| Case | First completed Qt paint ms A→B | Group wall p95 ms A→B | CPU core-equivalents A→B | Private MiB mean A→B |
| --- | --- | --- | --- | --- |
| Duck1 | 122.852→167.014 | 0.8707→0.9053 | 0.276→0.282 | 33.441→36.040 |
| Duck4 | 135.416→161.172 | 2.9149→3.1348 | 0.526→0.585 | 37.071→39.578 |
| Duck16 | 128.517→160.728 | 10.0105→10.7995 | 1.587→1.712 | 45.298→47.074 |
| Party1 | 128.094→143.970 | 0.9946→0.9940 | 0.320→0.282 | 39.385→41.113 |
| Party4 | 176.021→153.269 | 2.9726→3.1595 | 0.510→0.564 | 41.936→44.027 |
| Party16 | 127.631→158.049 | 10.4645→10.7867 | 1.480→1.414 | 49.418→51.304 |

Group wall time is the first render boundary through prepublication, not asset
preparation, per-image FPS or the whole UI update. CPU/private memory are for the
complete separate static production-linked probe, NOT the installed executable.
All six settled pause/hidden windows have zero new renders/readbacks and resumed
rendering returns. Paused visible Qt painting still consumes CPU. Steady new-batch
Qt paints are about60/s with p95 gaps18.258–19.436ms; no measured steady
prepublication group exceeds16.667ms. These are not monitor presentation, a GPU fence, a zero-CPU
guarantee or perfect60FPS evidence. Sample-boundary partial groups/image records
are retained, not silently forced to satisfy images=groups×count. Memory is higher
in every B row; this functionality stage claims no memory saving or general speedup.

B probe SHA256: `FEC00E408AD7A55213E57E48A9042581A50D3C98B975D214D4A9D9125DFDF66D`.
Raw series UTC2026-10-12T16:00:12.444370Z–16:06:05.728916Z, commands, PID/creation
identities, CSV/JSON hashes, analysis and idle receipt are preserved in the B root.
Original Duck/Party hashes are unchanged. Repeat only in a fresh build/output
namespace with the exact intended source pins and zero competing workers; reuse
the same instrument and observer, not the completed `matrix-001` directory.
The saved launcher at host
`build/checks/2026-10-12-own-mixed-geometry-001/run-baseline.ps1` deliberately
rejects reusing its evidence namespace.

## Checked local delivery

Clean frozen compilation revisions:

- AveMotion: `2b577a9626a976a1358eb78ddb64fc2924169994`.
- Avelabs: `47231515635c0a66658a7b5186e4b7577c508da2`.
- EXE SHA256: `58C6A522DCDC9BCAB42468973D77207E89FBF5C07BA440D158D0DE78BB85DCFE`.
- PDB SHA256: `1307674790550B42437B6AC4124452DE4E3FD3A2F94B10B052C26442B670955E`.
- Verified installation UTC: `2026-10-12T16:28:53.6046353Z`.

126 actual Release compile commands use `/MT`; the own-WARP reference-none link
contains no rlottie runtime. Root reviewed-script preflight and Execute passed,
copying only EXE, metadata and paired PDB. All26 package files/24unchanged notices
match the candidate; installed release/readiness/stability/manifest/SymChk passed.
Full previous26-file/PDB backup:
`D:/rvc/c++/DragonianVoice/Avelabs-UI/build/checks/2026-10-12-own-mixed-geometry-001/promotion-001/backup`.
Previous EXE17AA33DB491217401C5BDC7F73F1D0C19086AF80FC130FCBEA04D5C8C8ABDC41 /
PDBB2DAB8CB36D733F8A6532D9005487D081BA64073EEC74EC9325E957E81529C70 remain in
that verified backup and the earlier protected backup. No rollback was required.

`promotion-001/installed-after.json` SHA256:
`79BDF1C530DF478066155E7323E97C4B6D07A3F98ADBC418EC588DB13E134E92`.
It binds the actual package, backup, compiled revisions and reviewed audit/script:

- Host gate binding: `0FF8F51938ED5C8309877BDF9A7209726ECAF9E26905B17C3CFBC459D33B7309`.
- Delivery audit binding: `586859F4041F9B3306C5EB394A09AF5D2695D65FDB8FC54DF15CAE21879D7DBE`.
- Promotion script: `70D10BA1CE4F90E932BEF711DB15E927DF29D777D6FB909204E782DBF5389796`.

Exact configure/build/CTest/consumer and measurement arguments are retained in
the stage `*.command.json` receipts with actual exits and logs. The full engine
commands used `ctest --test-dir <preview> -C Debug --output-on-failure --no-tests=error --parallel 1`
and `ctest --test-dir <package> -C Release --output-on-failure --no-tests=error --parallel 4`,
under command-local VsDevCmd. Preview is the canonical
`C:/Users/USER/Desktop/AveMotion/out/build/windows-msvc-win32-preview`; package is
`C:/Users/USER/Desktop/AveMotion/out/checks/2026-10-12-own-mixed-geometry-001/package`.
Later handoff documentation commits are NOT compilation revisions and require
no product rebuild. Historical paths and all failed attempts remain intact.

## Acceptance limits and next step

Automated snapshots and controlled Qt scales do not close live physical-monitor
appearance, pacing or mixed-monitor DPI acceptance; the user has no PC access.
The limited mixed profile is not full Lottie/SVG support. WARP is software, not
hardware GPU. Existing derived-code notices and distribution obligations remain;
reference-free linking does not establish fully independent licensing or authorize
new redistribution. No external artwork, dependencies or license decisions changed.

This stage is closed with those limits. Do not repeat the completed tasks or
start another optimization investigation. The test-barrier P2 needs a separate
small deterministic follow-up; the next functional stage should choose one common
composition capability from the verified corpus, not a sticker-specific renderer.
No new scheduler, large cache, GPU, docking or physics work was included.
