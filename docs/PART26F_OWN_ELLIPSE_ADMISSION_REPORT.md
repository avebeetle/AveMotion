# Part26F — own ellipse admission

Status: accepted, 2026-09-27. Tasks1/2, final gates and independent review complete.
Stage baseline7000dd0b7470a17fd21ad3515a44eb7f140adae9.
Tested HEAD880020d4ab9d6199009cf4ecf16b5da5c6caf6aa; last product/test9269b8e.

## Scope and destination

This stage connects the existing first-party JSON reader to the exact, narrowly
supported ellipse admission grammar through one compiled first-party core. The
Telegram frontend keeps its syntax/scalar/resource compatibility contract; the
own-document frontend follows the explicitly different reader contract. It is
not full Lottie support, an own model, reference-free playback or GPU rendering.
No production Runtime route is changed. The eventual host is a statically linked
Avelabs-UI EXE, not a separate AveMotion DLL; the accepted UI build is untouched.

## Accepted work

Task1 product4c9237d extracts decimal arithmetic, grammar and owned materialization
from the old frontend into a private bounded borrowed-table core. Child links
preserve source order even with noncontiguous table rows. The frontend retains its
DOM and completed raw-event owners until the synchronous core call returns.
Existing admission/input headers and tests are unchanged. Independent task review
approved spec and quality with no findings; handoff6d9e9ca was ordinarily pushed
and remote equality verified.

Fresh September27 worker verification: full none33/33 (6.67s), Telegram focused6/6
(1.40s), stream/lifecycle2/2 (10.98s). Root committed-code core/compat2/2 (0.11s).
Raw evidence: out/part26f/task1/resume-20260927 and root-752c2d9-focused.log.
These are Task1 checks, not final F platform acceptance.

Task2 productb44dedf adds the private own-document projection and independent
descriptor/lifetime/resource/TGS/concurrency tests plus an explicitly policy-aware
Telegram comparison. Task review found three missing precedence witnesses;
test-only9269b8e adds them, with focused none/TG1/1 each and accepted scoped review.
No product defect or new behavior was required for this coverage correction.
Worker full scratch none34/34 and TG selected7/7 passed. Root filled the omitted
C/D verification with four relinks and own2+C/D4 tests,6/6 (10.90s), retained at
out/part26f/task2/root-b44dedf-regression.log. No task finding remains open.

## Evidence and interruption limits

Original September24 functional core-stub RED/GREEN and invalid-token correction
RED/GREEN remain under out/part26f/task1. That worker stopped on an unavailable
model error before its report/commit; the replacement inspected and completed the
same changes rather than recreating them or inventing a new test-first history.
Its fresh unquoted cmd regex failed before CTest and is retained; the correctly
quoted run passed. Incremental builds are not claimed as clean rebuilds.

The replacement accidentally committed its ignored SDD report in752c2d9. Handoff
6d9e9ca untracked that file without deleting or changing local bytes; history is
append-only. Raw evidence and SDD artifacts remain locally retained. No warnings,
historical failed commands or unresolved broad-stage work are erased by a passing
focused check.

## Decisions and final acceptance

The six detailed rulings and costs are recorded in
docs/superpowers/ledgers/2026-09-24-own-ellipse-admission.md: one shared compiled
core (bounded projection cost); distinct old/own policies (two explicit contracts);
document-only seam (later caller composition); retained raw owners/full alignment
before normalization (different internal storage/timing); direct-main/raw retention
(less isolation); corrected sibling-swap expected first error (fixture-specific
witness maintenance). No speedup, zero-allocation or race-detector claim is made.

Task2 independent descriptors, numeric/Unicode/path distinctions, TGS composition,
lifetime and two-thread functional isolation are covered; this is not TSan.

| Final root gate | Result |
| --- | --- |
| windows-msvc-direct2d, none |34/34,5.19s|
| windows-msvc-telegram-debug |80/80,91.34s|
| windows-msvc-win32-preview |74/74,94.05s; capture/WARP/preflight/selftest included|
| windows-msvc-samsung-debug |Configure/registration only, no Samsung all-green claim|
| Vendor/committed corpus, TGS integrity |Both vendor trees preserved;16 assets verified|
| Actual core/own include traces |Six invocations;103/102 headers each, only expected first-party/MSVC/SDK|

All built suites have zero skips/failures. These are fresh configure/incremental
build/CTest executions, not clean rebuilds or performance comparisons. Exact
commands, outputs, exits, CTest listings/JUnit, source/config/instrument hashes,
graphs and original-object hashes are retained in out/part26f. Final directories
are `880020d...-<preset>-gate-002` and `880020d...-include-trace-002`.

Commands, from the repository root (tag is the full tested HEAD above):

```powershell
& .\out\part26f\run-preset-gate.cmd windows-msvc-direct2d $tag
& .\out\part26f\run-preset-gate.cmd windows-msvc-telegram-debug $tag
& .\out\part26f\run-preset-gate.cmd windows-msvc-win32-preview $tag
& .\out\part26f\provenance-gate.cmd $tag
& pwsh.exe -NoProfile -File .\out\part26f\trace-includes.ps1 -EvidenceTag $tag
& pwsh.exe -NoProfile -File .\out\part26f\verify-boundaries.ps1 -EvidenceTag $tag
```

An initial boundary verifier used a substring which also matched the own adapter
name. Root reproduced the false positive on actual archive blocks; a7-case
actual-guard RED/GREEN regression and independent scoped review accepted an exact
leaf-name check. Because that script is hashed, all gates/traces were repeated
with the corrected instrument. Attempt001 and the failure remain retained.
No product correction or test weakening was needed; cost was an extra full gate
sequence, not an unexplained engine regression.

The final identity covers896 product/config inputs and five instruments. None
has zero rlottie source/enable records or vendor link edges. Telegram Runtime
remains reference-linked; vendor search paths do not mean actual included headers.
Root inspected full link/rule/variable closures, all six raw header inventories
and hashes, private installation/export rules, extraction and protected diff.
See out/part26f/root-final-boundary-review.md and the final private-boundaries JSON.
All new interfaces remain private, and old admission/input headers/tests and E
remain unchanged. Samsung's known golden failures are not fixed or hidden.

Whole-stage review7000dd0..880020d found no source findings. All its platform/
provenance evidence dependencies are now resolved by root; scope exclusions remain
explicit. No open/deferred F finding. Avelabs stays clean712d454, UI/out absent,
accepted EXE SHA256 C92F26EE8FEB4FF03F6DC7F4AFFF4B11FB48142743D1EDCCB4E213AAA81F82C2.

Next: separately design/build an own immutable model and canonical resources,
with a distinct own publication boundary, without forging or weakening the
existing reference certificate. Then prove own frames/pixels and integrate that
path into an isolated static Avelabs build. F alone is not own playback.
