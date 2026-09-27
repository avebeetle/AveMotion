# Part26F — own ellipse admission

Status: in progress, 2026-09-27. Tasks1/2 accepted; final gates/review pending.
Stage baseline7000dd0b7470a17fd21ad3515a44eb7f140adae9.

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

## Decisions and remaining acceptance

The six detailed rulings and costs are recorded in
docs/superpowers/ledgers/2026-09-24-own-ellipse-admission.md: one shared compiled
core (bounded projection cost); distinct old/own policies (two explicit contracts);
document-only seam (later caller composition); retained raw owners/full alignment
before normalization (different internal storage/timing); direct-main/raw retention
(less isolation); corrected sibling-swap expected first error (fixture-specific
witness maintenance). No speedup, zero-allocation or race-detector claim is made.

Task2 independent descriptors, numeric/Unicode/path distinctions, TGS composition,
lifetime and two-thread functional isolation are covered; this is not TSan. Final fresh full none,
Telegram, Win32-preview, Samsung configure/provenance, actual include/link/install
closure and independent whole-stage review are required before F acceptance.
Samsung is configure-only here, not a claim that its known golden failures are fixed.
