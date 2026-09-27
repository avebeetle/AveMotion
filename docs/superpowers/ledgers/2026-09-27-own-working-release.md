# Part26L durable ledger

Plan docs/superpowers/plans/2026-09-27-own-working-release.md; spec1bc19f7.
Bases engine a816d09 / host f452922; main clean at preflight. User explicitly
requests working build/Release, superseding K output protection for this stage.
User selected controller choice of next example: primitive_geometry, staged
separately after L. No real downloaded rights-cleared sticker currently exists.

| Task/pair | Checked contract | Disposition |
| --- | --- | --- |
| Task1 self | package flag, exact layout, entry, commands, tests | one coherent own-only host route; no engine implementation |
| Task2 self | backup, verified candidate, canonical install and recovery | root only; doesn't call installation new feature coverage |
| Task1/2 | own metadata26-file profile, intermediate EXE/PDB, install components | candidate staged first; canonical prefix does not ban deliberate staged validation |

Ruling: Work on existing main and keep raw/SDD — explicit user choice — cost is no branch isolation and retained disk use.
Ruling: Explicit own-working flag permits host package only; other labs retain guards — enable requested EXE without reference distribution — cost is another build-mode invariant to test.
Ruling: Preserve existing notices and mark local-development-only, select no public license — current repository licensing is unresolved — cost is no claim of public distribution readiness.
Ruling: L working-package first, then whole primitive_geometry support — separates reversible installation from a wider own-model change — cost is the first installed version still supports only the existing ellipse subset.

Ruling: Routine diagnostic commands still install the successful own build into
the canonical package/PDB; Task1 tests fake external execution rather than
running that install — the user wants ordinary and diagnostic builds to update
one working EXE, while Task1's prohibition concerns test execution — cost is
that subsequent real installs still require the documented backup discipline;
this is not a new atomic updater. Compilation itself remains intermediate.

Spec self-reviewed/committed1bc19f7. Plan self-reviewed and controller-approved;
SDD selected under delegated authority. Task1 implemented/reviewed; Task2 pending. Root owns
Git/promotion, no writer may touch canonical Release/PDB before root backup.

Task1 host699fc0b: functional RED/GREEN; deployment95/95, build29/29,
boundaryPASS, referenceCTest3/3(Qt42), own4/4(Qt53), working-entry4/4(Qt53).
Root fresh verifier19/19 and diffcheckpassed. Task review found no product
defect; one mandatory-file-list deviation below and pre-existing warnings.

Ruling: Accept unchanged DiagnosticSymbols.cmake/test_diagnostic_symbols.py/
test_build_boundary.py instead of mandatory edits — existing lab PDB routing
already keeps outputs intermediate, original guard suites pass unchanged and
new own-package cases are in test_own_release_build.py — cost is coverage
distributed between suites rather than the originally enumerated files. Root
checked existing routing and guards; no dummy edits or duplicated tests.

Task1 complete f452922..699fc0b with that explicit accepted brief deviation.
Minor deferred: existing CMake unused-overlay warnings and UI/vendor compiler
warnings remain, with provenance/counts in task report. Ancillary configure
logs were reused; aggregate RED/final logs remain; no claim every intermediate
per-case file is an immutable record. All actual static app/import/PDB/backup/
promotion checks remain Task2, not inferred from mock/staged fixture checks.
