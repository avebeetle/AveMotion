# Own vector metadata and separated position

This bounded compiler stage adds validated exporter annotations and separated
layer position through the existing own model and evaluator. All three fresh
correctness gates pass and independent task/whole-stage reviews approve this
bounded block with no Critical/Important issues. Product commit:
`d4157cae8f2237649d127d139109c5cf5b984765`. The installed
Avelabs Test Animation EXE remains unchanged; full
original Party playback and Release performance measurements are separate gates.

## Corpus driven contract

The unchanged local Noto Party Popper JSON has SHA256
`EAB1FD7070BFFD2965C12A80D2D91CFF76683AF65019F142ACF9A553EE877B5C`.
Its observed non-rendering fields are `np/cix/ix/mn` on groups, `ix/mn` on
paths, `mn` on fills, and a root marker `{tm:25,cm:"rest",dr:0}`. It contains
no `l2`. The original artwork stays outside Git and distribution, under the
existing local evaluation and attribution policy in
[the corpus report](REAL_CORPUS_2026-10-08.md).

The compiler keeps its strict context-specific allowlists. Optional group
`np/cix/ix` and path `ix` must be exact integers in 0..32768; `mn` is a string
of at most 1024 UTF-8 bytes. `np` is not treated as structural item count.
Only group, path and solid-fill contexts admit the witnessed names. Other
locations, unknown fields and unwitnessed `l2` remain rejected.

Root `markers` admits 0..128 annotation objects with only required `tm/cm/dr`.
Time is finite in -20000..20000, duration finite in 0..20000, and comment is a
string of at most 1024 UTF-8 bytes; fractional values are allowed. Valid markers
are discarded and have no scene or playback effect. This is not marker seeking
or a new transport API.

Non-precomp shape/null layers may have `ks.p` with only
`{s:true,x:<scalar property>,y:<scalar property>}`. Both axes are required,
coordinate bounds stay -32768..32768, and existing temporal-track/easing/budget
rules apply independently. The compiler emits `TransformPositionX/Y` records
and their separated-dimension flags, not a combined position. Group/precomp
transform restrictions and the no-auto-orient policy remain unchanged.

## Telegram behavior and reuse

The pinned Telegram reference is
[`67f103bc8b625f2a4a9e94f1d8c7bd84c5a08d1d`](https://github.com/TelegramMessenger/rlottie/tree/67f103bc8b625f2a4a9e94f1d8c7bd84c5a08d1d).
Its [parser](https://github.com/TelegramMessenger/rlottie/blob/67f103bc8b625f2a4a9e94f1d8c7bd84c5a08d1d/src/lottie/lottieparser.cpp)
discards these annotations and supports separate position axes. Our existing
Telegram model bridge already emits scalar X/Y records, and the own property
evaluator reconstructs position from them. This stage reuses those contracts;
it does not copy the parser's blanket unknown-key skipping or key-order-sensitive
split parsing, replace a dependency, or change licensing.

The separately inspected newer
[lib_lottie at fbb922c](https://github.com/desktop-app/lib_lottie/blob/fbb922c30b7c30ba17ad5fd4fcb989eadd620062/CMakeLists.txt#L62)
links `external_tlottie`. The pinned rlottie is a stable differential baseline,
not a claim that our dependency matches today's Telegram release. No backend
or playback policy is changed by this compiler stage.

## Verification

Functional RED preceded production code: a successful test build followed by
three separately reported failures at metadata `/layers/0/shapes/0/np`,
root `/markers`, and split `/layers/0/ks/p/s` (CTest exit8). Initial command and
test-authoring compile mistakes are retained separately, not counted as RED.

Focused checks cover independent transform expectations, malformed-input atomic
rejection, full-field ordinary scene comparison and WARP pixel comparisons on
owned fixtures. New string-cap tests use ASCII decoded-byte boundary witnesses;
there is no dedicated multibyte/escaped cap regression. Task review verified
that the existing JSON reader supplies decoded bytes to the production checks.
Existing goldens and tolerances are unchanged. An incorrect
test expectation initially counted a shifted precomp child in root frame0;
the expected count was corrected to one, with annotation-neutrality assertions
retained. No production visibility rule was changed.

Fresh full builds and serial Debug CTest pass on frozen product bytes:

| Existing MSVC build | CTest result | Duration |
|---|---|---|
| windows-msvc-direct2d, no reference | 47/47 | 9.94s |
| windows-msvc-direct2d-capture, Telegram | 95/95 | 231.64s |
| windows-msvc-win32-preview, Telegram graphics | 96/96 | 220.73s |

All build and test exits are zero, with no CTest skips. The preview selftest
records WARP, one resize and one device recreation. These durations are
correctness gate timings under a foreign workload, not Release benchmarks.
Independent task review is Approved with no Critical/Important issues. Its minor
coverage-description correction is incorporated above without changing frozen
product/test bytes. The final whole-stage review accepts that disposition and
finds no required fix; the missing cap-specific Unicode witness is optional
follow-up work. Reviewers did not rerun the gates; the final review relied on
the frozen patch, design/report/ledger, prior task review and controller's hash
confirmation. Completion is for this compiler block, not the full engine.

New owned fixtures cover 2400 full-field ordinary scenes (three fixtures,
800 samples each) and 210 WARP capture cases per graphics gate (two fixtures,
105 cases each). The capture comparisons have zero different same-backend
pixels and pass the unchanged CPU-reference policy; this is not a claim that
CPU and WARP rasterizers produce identical pixels.

Both unchanged original Ducks pass the final model inventory, full-field
differential and capture commands. Each has 2960 scene samples and 68 exact
same-backend WARP rows, CPU policy passing, no skips. a2 remains 50 layers,
52 draws, 94 tracks; a3 remains 37 layers, 36 draws, 33 tracks. Their SHA256 pins
remain `2CDBF4D03DE421B6E0BC73871F4FC7ABB685EF3B3B30B581E8AB75C71501B15A`
and `7CD55718288EB1B1D1EDFD0E774D9038E076A7F5AADF9835A92CF942948A4BC1`.
Canonical/projected comparisons use the existing unchanged tolerance; raw local
geometry is not identical to the reference and retains previously documented
differences. These CLI regressions are not native host acceptance or performance.

Reproduce in the existing MSVC developer environment at the canonical source:

```powershell
& 'C:/Program Files (x86)/Microsoft Visual Studio/2022/BuildTools/Common7/Tools/Launch-VsDevShell.ps1' -Arch amd64 -HostArch amd64 -SkipAutomaticLocation
cmake --build out/build/windows-msvc-direct2d --config Debug --parallel 4
ctest --test-dir out/build/windows-msvc-direct2d -C Debug --output-on-failure --no-tests=error -j 1
cmake --build out/build/windows-msvc-direct2d-capture --config Debug --parallel 4
ctest --test-dir out/build/windows-msvc-direct2d-capture -C Debug --output-on-failure --no-tests=error -j 1
cmake --build out/build/windows-msvc-win32-preview --config Debug --parallel 4
ctest --test-dir out/build/windows-msvc-win32-preview -C Debug --output-on-failure --no-tests=error -j 1
```

Raw commands/output and frozen source hashes belong under
`out/own-vector-metadata-position-2026-10-11/`. Existing caches are Visual
Studio2022 x64, source root C:/Users/USER/Desktop/AveMotion; reference-linked
install remains OFF. No configure, dependency or vendor version change is part
of this stage. Existing vendor warning noise remains disclosed in the raw logs;
successful CTest is not a warning-free build or a complete Lottie/SVG claim.

## Installed Release measurement limitation

The existing installed EXE has SHA256
`FC3AEC2BB876BCF4C2F62F441BBD366C5CB728D2F75F14B3684990E52E6D6E4E`,
engine build 16a8cdef and host build 59021fd. Its own WARP renderer is software,
not hardware GPU rendering. The October11 native measurement attempt launched
one owned process, but capture timed out twice and the UI click had no capture
geometry. Only that owned process was stopped after verifying path and creation
time. No 1/4/16 samples or native acceptance were obtained, and host files were
not replaced. A foreign AveVoice test workload also prevents clean performance
comparison; it was not interrupted.

Existing diagnostics already expose produced images, latest-batch render time
and mailbox delivery/replacement. Those are not presented FPS or cumulative CPU
time. The host's process observer can measure CPU, memory and handle deltas on
a creation-pinned process. A future uncontended visible-UI run must retain fixed
physical per-instance size and record requested, bounded and actual raster sizes
and DPR: at 16 instances a viewport above 512 by 512 may hit the total-area limit.

## Next useful steps

This block does not accept stripped Party as a substitute for the original.
Full original Party still needs separately designed gradient fill and alpha
matte compositing, followed by unchanged-input scene/capture and host gates.
No renderer switch, new animation format, scheduler rewrite, blanket fallback
or unsupported performance claim is warranted by this stage.
