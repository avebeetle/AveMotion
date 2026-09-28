# Part26O — Duck X3 engine parity

Task3 engine work independently reviewed and accepted; same-host delivery remains.
The canonical Part26N host package has not been changed by this task.

## Input and independent correspondence

Unchanged local `a-2.tgs`, TelegramStickersImport commit
`b8951c8a02b245142142d341644d0756cf16278c`, `Example/Source/a-2.tgs`:
SHA256 `2CDBF4D03DE421B6E0BC73871F4FC7ABB685EF3B3B30B581E8AB75C71501B15A`.
The artwork and raw captures remain ignored local evaluation inputs.

Reference execution instances are enumerated from a separately parsed pinned
Telegram model, using its reverse authored layer traversal. The complete scene
is checked against that map, including inactive layers, structural edges,
visibility clocks, and unique execution IDs. Definition roles use composition,
authored layer identity, and child-item ordinals. Own roles use the compiler's
immutable JSON-pointer/containing-instance sidecar. These maps join only after
independent construction; definition paint IDs alone cannot identify an instance.

Reference matrices are keyed by execution `draw.modelNode`, not shared paint
source ID. Enclosing reference clip rectangles derive from reference ancestry,
dimensions and evaluated transforms. Each reference draw must independently fit
every ancestor clip's conservative raster hull before own clip omission is
accepted. No required reference clip is removed or normalized.

For an outer stroke, reference local geometry is in paint space while own raw
local geometry is in path space. The predictor is reference path-group world
times inverse reference paint world. Raw differences remain reported; the large
outer-stroke differences are coordinate-space differences, not the small N
matrix-cancellation rounding residual. Exact topology and final geometry,
the absolute `1e-4` numerical limit, exact WARP pixels, and CPU policy remain.

## Narrow product corrections

At a-2 frame4 the previous emitter published two zero-opacity layer draws:
reference23 versus own25. The time-visible layers and their transforms remain
present, but publication now follows pinned `abs(combinedAlpha) <= 1e-6` semantics.
Transform-parent opacity remains separate from structural opacity.

At512 pixels, direct diagonal stroke-scale arithmetic differed from pinned
mapped-point subtraction by about `5.72e-5` in one pen width. This produced14
different final pixels with maximum channel difference5, despite numerical and
CPU acceptance. Restoring translation-inclusive float subtraction, while keeping
the distinct paint-scope binding, eliminated those differences. No Direct2D
backend, parser, vendor, golden, dependency, Windows or policy edits were made.

## Evidence

Final scene/capture/cost results and the fresh129x129/frame0 ordinary-reference
BGRA handoff are recorded in
`out/part26o/task3/final-provenance.json` and the Task3 implementation report
`.superpowers/sdd/2026-09-28-own-duck-x3/task-3-report.md`.
Raw RED/GREEN evidence is preserved under `out/part26o/task3`.

Final engine gates: no-reference47/47 (7.73s), Telegram100/100 (112.43s),
explicit Win32-preview96/96 (136.31s). The preview/real gates rebuilt the final
source after removal of an unguarded compiler macro from the unused costs-output
branch; the Telegram gate predates that output-only portability correction.
Original a-2:2960scene comparisons, final path/matrix max1.52588e-05,
stroke max2.47955e-05, predicted local representation max0. Both a-2/a-3:
68/68 exact WARP captures and68/68 under unchanged CPU policy. Four original
a-3 model/stream regressions also pass. Large raw outer-paint local-coordinate
differences remain disclosed, not counted as floating-point rounding.

The exported129x129/frame0 reference is66564bytes, SHA256
`6EF148763357E01B286695B2E3E9840475665131F6A697EE782B01B3249EFDF5`.
It is for a-2 only; its input/build hashes are in final-provenance.json.

Uncontended MSVC Debug timing on Ryzen5 5500, Windows10 19045; five iterations:

| Measured phase | Range, milliseconds |
| --- | --- |
| JSON reader |4.890–5.181|
| Own compiler |280.224–282.474|
| Prepared asset |3.157–3.794|
| Stream creation |0.706–0.776|
|720scene emissions across four targets |1097.509–1133.680|

Each replay published17904draws/180912path points and720changed sequences.
A separate retained-evaluator180frame probe measured118440property visits,
6601cursor hits,311adjacent moves,43binary searches and14555shape-point
interpolations. These are not hidden stream/allocation counters; the timing
is not end-to-end Qt/WARP playback or a Release/speedup benchmark.

Inherited Telegram header warning C4251 remains disclosed. These are MSVC Debug
and software WARP checks, not Release speed, hardware, allocation, DPI, race or
native-host interaction claims. Task2's preparation-clock fixture-strength minor
remains for controller triage: this task did not touch preparation or that test.
Task3 review also notes a missing count guard in failure-only per-draw capture
diagnostics; current passing equal-count evidence is unaffected. Both minors are
carried to the whole-stage review before delivery. Root fresh default-scene
CTest1/1 and original a-2 full2960scenes passed after the final source freeze.

## Same-host test checkpoint

Host source `2b9982ff9312df80b4f7cdc9887d72860c93adff` adds only three test
seams and its handoff report. Fresh /MD working-own4/4,opt-in-own4/4,
reference3/3 passed; explicit required a-2 Qt tests have no skips, and original
a-3 focused controls/rendering pass in both own trees. Root worker3/page3/
renderer4 passed independently. Python deployment97/diagnostics34/build29/
runner3 are green. A fitted1x1 a-2 frame is still rejected by the unchanged
conservative clip policy; tests verify null/no new target or readback/exact129
recovery and retain a-3 tiny-target success.

Independent Task4 review and scoped fix1 review accepted O operational scripts
after a real inert RED exposed insufficient linker-log target identity. Exact
app/smoke project, intermediate directory and output binding now passes19/19
guards (also root19/19), recovery2/2 and dual-original smoke-proof4/4. This is
script preparation, not an actual static audit or promotion. Real-byte hash
coverage in inert smoke tests remains a minor for final triage.

Canonical N EXE/PDB remain untouched. Whole-stage review, fresh frozen /MT
candidate, actual bound source/link/import audit, full26-file/PDB backup and
native acceptance are still pending. Host report:
`D:/rvc/c++/DragonianVoice/Avelabs-UI/docs/testing/own-duck-x3-2026-09-28/REPORT.md`.
