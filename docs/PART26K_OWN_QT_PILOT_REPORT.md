# Part26K — own animation in Avelabs Motion Lab

Date: 2026-09-27. The bounded own Qt pilot is complete. This report is not a
release/package or full sticker-compatibility claim.

## Result and identity

The existing Avelabs Motion Lab now has a compile-time `own-warp` route:
own JSON/TGS preparation -> own ellipse playback -> existing Player -> own
scene/plan -> Direct2D/WARP -> owning QImage -> existing Qt canvas. No second
scheduler, worker, timer, public SDK or silent reference fallback was added.
Lab remains OFF by default; its default renderer remains `reference-cpu`.

The supported input is the existing narrow one-ellipse/fill/position-animation
subset, not arbitrary SVG/full Lottie/downloaded stickers. WARP is software
rendering with pixel readback; this is not hardware GPU presentation or proof
of Telegram-level performance. The original reference mode is preserved.

| Identity | Commit |
| --- | --- |
| Host stage base | `712d454a7c5c175ad59a3ca547c6b22ce8392da1` |
| Host integration | `7e0ec588675ab3d501f01c0eae82767f714c969b` |
| Host test-only review fix / frozen tested HEAD | `e701e3fc357d99bf3c4a65553c43e3a2c41119a0` |
| Host documentation handoff (ordinary-pushed) | `f452922dce98f16b80490eb86bbf7d5c4899efec` |
| Engine stage base | `1bab64b` |
| Engine frozen tested HEAD (only stage documentation changed) | `ecac1a585d96931fcc6220c992353c06be701026` |

Subsequent handoff commits change documentation only. Source hashes taken
before/after every final gate bind results to the product above, not to newly
rebuilt binaries after a documentation seal. Both normal main checkouts were
used under explicit user delegation; only the controller committed/pushed.

## Verified behavior

Private host glue reuses existing own parser/model/prepared-asset APIs and
strict limits: host input 2 MiB, own JSON 1 MiB/4096 values/depth32, strict TGS
decode. Unsupported/malformed replacement preserves the old usable session.
The committed basic JSON is unchanged; test TGS is generated from those exact
first-party bytes with Python stdlib. Existing repeater TGS is a negative case.

One WARP device and size-reusable target/staging pair serve actual rendering.
Every frame clears transparency and checks draw coverage/native results;
row-pitch-aware BGRA copies own their QImage storage. Tests cover opaque,
transparent and translucent premultiplied bytes, movement, odd widths, retained
images after render/resize/destruction, inactive clear, and resource counters.
Successful replacement retires planner/backend caches without a new scheduler.

Existing controls, 1/4/16 instances, visibility Freeze/user pause, pixel budget,
generation cancellation, bounded mailboxes and shutdown are exercised in both
modes. A real viewport refusal during a four-image batch verifies no partial
publication, one error, no automatic retry loop, and explicit-load recovery.
The test uses approved private friend access, not a product failure switch.
Native HRESULT/device-loss recovery was not injected or runtime-certified.

## Fresh final gates

All entries below executed on the frozen heads above; no failures, skips or
disabled tests. QtTest pass totals include init/cleanup, not that many separate
feature scenarios. Builds/tests and GUI cases were run sequentially.

| Gate | Actual result | Raw evidence directory |
| --- | --- | --- |
| Engine `windows-msvc-direct2d` (none) | CTest 42/42, 5.18 s | engine `out/part26k/windows-msvc-direct2d-001` |
| Engine `windows-msvc-win32-preview` | CTest 87/87, 106.34 s, including capture/WARP/own capture/preview | engine `out/part26k/windows-msvc-win32-preview-001` |
| Host reference standalone /MD | CTest 3/3, 6.45 s; QtTest worker31 + page11 = 42 | host `build/checks/part26k-final-reference-001` |
| Host own standalone /MD | CTest 4/4, 7.13 s; QtTest worker32 + page11 + renderer10 = 53 | host `build/checks/part26k-final-own-001` |
| Actual build-boundary matrix (Task1) | exit0, 31 PASS checks | host `build/checks/part26k-boundary-final2`; summary `build/checks/part26k-task1/boundary-final2.log` |
| Fresh full static app + own smoke /MT | build exit0; CTest 1/1, 0.09 s; both import audits PASS | host `build/checks/part26k-static-001` + `part26k-static-completion-001` |

The engine gates configured and incrementally built (ninja reported no work),
then ran the complete suites afresh. They are not clean rebuilds. Their 923
selected tracked source/config/vendor/provenance hashes were stable. Host
standalone gates built all targets incrementally and checked fresh QtTest output
timestamps and unchanged selected inputs. The static app uses a fresh tree.

The boundary matrix checks actual OFF/reference/own configurations, ON/OFF bad
selectors, conflicting variants, source/CRT/cache/fresh-tree guards, both static
smoke links/runs, and refused installs with no output prefix. The existing
EXCLUDE_FROM_ALL Reference declaration is permitted; actual own build/link must
exclude it. A full repeated Samsung gate is not claimed for this host-only step.

Independent read-only artifact audit accepted actual static closure. App graph
has 12 projects/46 edges; smoke 11/44, each with nine engine-side static
libraries and no rlottie/Reference dependency. Actual app link-read has172
non-header inputs (73 libraries/95 objects/4 tool-OS reads); smoke84
(70/10/4), neither containing rlottie or avemotion_reference. Eight AveMotion
libraries reach final links; miniz is absorbed into Formats. Compiler records
explicitly disable reference rounding and rlottie (`=0`); this is not absence
of every textual reference token. All compiling projects use /MT; app/smoke
import tables pass with 34/32 allowed Windows imports, no delay/Qt/CRT DLL
imports. Runtime LoadLibrary and distribution/licensing are outside this audit.
No cmake_install.cmake or reference/rlottie library artifact was generated.
Detailed record: engine `out/part26k/static-artifact-audit.md`.

All 113 actual compiler commands across 11 tlogs use /MT, not /MD; the
independent audit also compared all 517 selected manifest tuples with zero
differences/duplicates. These are observed build boundaries, not new guarantees
about every possible runtime load or every file on disk.

The static app is 52,568,064 bytes, SHA256
`613550DFCE84146A3B68988FCCFD73D68D94934CAAE71594D4DA544B737F3A2F`.
Own smoke SHA256
`485A3AAD2D89F53D53A6199919DAA66E085FBA54DE29F73B10C18C57C7C3BE98`.
The app is in host `build/cmake/part26k-own-static/Release/AvelabsUI.exe`;
smoke is under `tests/motionlab/Release` in the same tree. Full app build took
276.72 s, not animation latency. Four static build warning lines are unchanged
UI C4100/C4505; configure warns the overlay variables are unused with
installation disabled. These were not suppressed or treated as product failures.

## Visual and bounded measurements

The controller viewed fresh Qt page grabs `motion-page-own-0.png` and
`motion-page-own-1.png` in `build/checks/part26k-final-own-001`. Both show the
existing Voices page, exact badge `Experimental — own ellipse / WARP readback`,
loaded basic fixture, and cyan ellipse moving right as seek changes 0->1000.
These are real loaded QtTest page captures, not static EXE/native desktop/DPI
acceptance. No accepted UI package was launched or replaced for this gate.

Observed 752x72 page output: device/target1/1, successful readbacks1->2, reported
render time6,472,600ns then251,500ns. These are two individual observations,
not a benchmark, distribution, A/B comparison or speedup. Renderer assertions
also verify equal-size independent playbacks keep device/target1/1 and increase
readbacks; resizing rebuilds target2 while device remains1. No zero-allocation,
TSan, hardware-GPU or generalized performance claim follows.

## Review and failure history

Independent task review found one Important missing translucent raw-byte
premultiplication assertion. Test-only commit `e701e3f` fixed it; a straight-alpha
mutation failed the expected raw-channel ratio, then the exact product code was
restored and the scoped re-review accepted it. Final full suites include this
new test. The most-capable independent whole-stage review found no Critical or
Important issue; no final product fix/re-review was necessary.

One Minor remains: switching renderer in an already configured tree sees the
helper-cached variant as an explicit conflict. Fresh separate directories are
the supported documented workflow; in-place switching is deferred, not silently
called fixed. All declined judgments are explicit limits here: general sticker
coverage; hardware/direct presentation/performance; native fault injection;
new native DPI/docking/tray acceptance; unchanged vendor/UI warning cleanup;
installed/public SDK/package certification.

All functional RED and failed attempts remain in task raw evidence: compiling
null renderer and reference-bound worker failed before implementation; initial
build-selection target-spelling mistake; own-suite mode-specific expectations;
odd-width test literal corrected using aspect-fit arithmetic (not renderer or
golden changes). Fault-latch and straight-alpha mutations are postimplementation
witnesses, not retrospectively claimed test-first RED. Task1 reference build had
67 warning lines, own4, from unchanged vendor/UI files. No warning suppression
or unrelated cleanup was introduced. Complete task/review records are retained
in `.superpowers/sdd/2026-09-27-own-qt-pilot/`.

The original root static runner passed configure/build/CTest/app import audit,
then failed the smoke import audit because it incorrectly assumed the smoke
EXE was in root Release. Generated CTest and build output established the actual
tests/motionlab/Release path. Original failure/log/runner are retained unchanged
in static-001. `complete-static-gate.ps1` reran only the corrected smoke audit,
checked 517 selected source/config/test/tool inputs against the pre-build hashes
and the same frozen HEADs, and verified protected Release/out. No rebuild,
product fix, test weakening or repeated successful test was needed.

## Commands and preservation

Exact isolated configure commands are in Avelabs `docs/build/own-motion-lab.md`.
Final raw command JSON accompanies engine and static gates. Root runners are
retained under engine `out/part26k/`:

```powershell
# Engine: inside existing VS2022 x64 environment, each preset sequentially.
./out/part26k/run-engine-gate.ps1 -Preset windows-msvc-direct2d -Attempt 001
./out/part26k/run-engine-gate.ps1 -Preset windows-msvc-win32-preview -Attempt 001
# Both host suites, sequential; dynamic Qt PATH/plugin configured by runner.
./out/part26k/run-host-tests.ps1 -Mode reference -Attempt 001
./out/part26k/run-host-tests.ps1 -Mode own -Attempt 001
# Fresh build/cmake/part26k-own-static; static existing Qt6.10 SDK, /MT.
./out/part26k/run-static-gate.ps1 -Attempt 001
# Remaining audit only, after diagnosing the original smoke-path script error.
./out/part26k/complete-static-gate.ps1
```

These are recorded commands, not rerunnable-overwrite instructions: use new
evidence/build labels when repeating and preserve failed runs. The static
runner refuses an existing tree. No dependency installation, install/package,
Windows/security/power changes, automation, external corpus download, vendor,
license, golden, docking/tray/DPI behavior or accepted Release edits occurred.

Accepted `build/Release/AvelabsUI.exe` remains SHA256
`C92F26EE8FEB4FF03F6DC7F4AFFF4B11FB48142743D1EDCCB4E213AAA81F82C2`;
Avelabs `out` remains absent. Build and evidence remain under the permitted
isolated `build/cmake` and `build/checks` directories.

## Decisions and costs; next boundary

The six delegated rulings are preserved in the durable ledger: private host
seam (future revision cost); compile-time WARP readback (CPU copy/no hardware
claim); strict own subset (most stickers rejected); direct-main/root-only Git
and retained raw (no branch isolation/disk usage); actual link closure instead
of banning an excluded target declaration (transitive dependency audit needed);
private test friend for real viewport refusal (native fault injection remains
unverified). These are intentional limits, not hidden completion claims.

Next useful stage should extend the measured own feature subset toward a
provenance-cleared real sticker, reusing this very UI/worker/canvas for visible
acceptance and comparison with the preserved reference build. Do not create
another shell/scheduler, rewrite all Lottie, or start native presentation merely
to fill time. Coverage and output optimization remain separate measured decisions.
