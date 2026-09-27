# Own Working Release Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Put the own no-rlottie Motion Lab into the user's canonical Avelabs EXE with verified metadata, symbols, notices and recoverable replacement.

**Architecture:** Explicit host-only working-package mode; existing reference and isolated own experiments retain no-install guards. Normal/diagnostic entry points select own mode, existing UI page becomes available without a flag. Build intermediates first; root alone promotes a verified package.

**Tech Stack:** Windows x64, C++20, Qt6.10.0 static /MT, CMake3.25+, Python3.9+, existing MSVC2022 and dependencies.

**Spec:** docs/superpowers/specs/2026-09-27-own-working-release-design.md (1bc19f7).

**Execution outcome:** Tasks1/2 complete 2026-09-27. Host699fc0b+56dc299;
fresh candidate and installed canonical pair verified. The prospective steps
below are the original execution contract; actual commands/results, accepted
file-list deviation and native-GUI limitation are in
[the completion report](../../PART26L_OWN_WORKING_RELEASE_REPORT.md) and ledger.

## Global Constraints

- User explicitly authorizes build/Release replacement, ordinary commits/push in main; root-only Git and actual promotion.
- No dependency installs, Windows/security/power changes, automations, unrelated UI changes, vendor/golden changes or external asset downloads/publication.
- AVELABS_OWN_MOTION_RELEASE defaults OFF; only explicit own-warp/none/static x64 Release /MT/actual host root may use it.
- Generic lab defaults OFF/reference; reference and unflagged own labs remain no-install. AVEMOTION_ENABLE_INSTALL stays OFF.
- Working outputs compile into intermediates, not canonical EXE/PDB; only verified promotion changes build/Release and build/symbols.
- Exact own package26 files: EXE, metadata,20 original notices plus four spec-named copies. Original22-file profile still supported.
- No engine product changes in L; primitive_geometry coverage follows separately. Preserve raw/SDD.

## Review Focus

- A normal or diagnostic rebuild must not silently remove own integration: Task1 wrapper/preset/diagnostic command tests.
- Reference/foreign caller trying the working flag must fail before generating install rules: Task1 real configure negatives and existing scope guards.
- Source metadata says own but notices/revision mismatch: Task1 exact package profile refusals, Task2 byte/hash comparison.
- A failed build must leave canonical EXE and PDB unchanged: Task1 intermediate output assertions; Task2 backup/pre-promotion hash checks.
- Working page installed twice or disabled without CLI: Task1 actual Qt page-entry assertions, retaining isolated reference opt-in.

### Task 1: Own-only host package route, commands and page entry

**Files (host D:/rvc/c++/DragonianVoice/Avelabs-UI):**
- Modify CMakeLists.txt, CMakePresets.json, cmake/AveMotionLab.cmake, cmake/DiagnosticSymbols.cmake, build_ui.bat.
- Modify src/app/motionlab/MotionLabEntry.cpp (entry policy only).
- Modify tools/deployment/verify_release.py; tools/build/build_diagnostic.py.
- Modify tests/deployment/test_verify_release.py and test_release_readiness.py as needed; tests/build/test_build_diagnostic.py and test_diagnostic_symbols.py.
- Modify tests/motionlab/test_build_boundary.py, CMakeLists.txt, tst_motion_page.cpp; add tst_motion_entry.cpp if separating compile-time entry cases is cleaner.
- Add tests/build/test_own_release_build.py for wrapper/preset behavior if no existing home.
- Modify docs/build/own-motion-lab.md, diagnostic-static.md and original-static.md commands. Root owns current-release/output-layout/result updates in Task2.
- Small dedicated cmake/OwnMotionRelease.cmake allowed if it keeps package metadata/notices policy together, no generic framework.

**Interfaces:**
- Preserve `avelabs_add_motion_engine()` and `avelabs_configure_motion_target(target)` entry points; conditional narrow host-package validation in helper/root.
- New CMake BOOL/private app definition `AVELABS_OWN_MOTION_RELEASE`, default OFF. No runtime selector/API.
- Preserve `installMotionLab(MainWindow&, const QStringList&)`; own working compiled mode installs once without CLI, other modes still require --motion-lab.
- Preserve `verify_package(package: Path, dumpbin: Path) -> dict`; choose exact layout from valid metadata, do not add a permissive extra-files switch.
- Extend `build_diagnostic(..., parallel=2, engine_source=None)` and CLI `--engine-source`. If omitted use own-static configured cache; absent/invalid source fails, never fallback. Preserve existing positional calls and output/report safety.
- New presets own-static (build/cmake/own-static) and own-static-diagnostic (diagnostic intermediate); source cache initialized explicitly, not hard-coded or overwritten empty. Standard build_ui.bat static [absolute-engine-source] selects own-static. Legacy original presets remain explicit alternatives.

- [ ] Step1: Record host BASE f452922 and clean state. Read spec and existing release inventory. Run baseline deployment/build Python suites with existing Python3.9; raw under build/checks/part26l-task1. Never run real canonical build/install/diagnostic command during this task.
- [ ] Step2: Functional RED package tests: valid own metadata plus exact26-file inventory is accepted; missing any added notice, extra file, invalid/missing/duplicate Motion or revision, wrong distribution field fail; original22-file profile still passes. Mock dumpbin boundary only, use actual files/parser/layout. Save failures before modifying verifier. Implement exact profile selection and run focused+deployment suite.
- [ ] Step3: Real configure RED: own flag + valid config produces host Runtime/Symbols install rules and intermediate EXE/PDB; old helper currently suppresses/refuses it. Invalid flag with labOFF/reference/variant/CRT/prefix/foreign-caller fails. Implement narrow exception; retain all no-install deferred/cache/fresh-tree tests for nonworking lab modes. Do not install into canonical directories in tests. Engine SDK switch remains OFF and rlottie/Reference absent from executable link closure.
- [ ] Step4: Generate own metadata and four exact notice copies/install rules from spec. Valid canonical configured prefix is mandatory; stage Runtime install via explicit --prefix to fresh build/checks directory is allowed. Preserve old metadata fields and original mode. Engine revision is actual Git identity with dirty/unknown reporting; final promotion requires clean pinned identity. Runtime install must never include SDK, reference, DLLs, PDB or arbitrary engine files. Symbols component places corresponding PDB in build/symbols only during root promotion.
- [ ] Step5: Wrapper/preset/diagnostic functional RED: inspect real generated commands with fake tool boundary or fixture executable, not grep-only tests. Assert normal/diagnostic uses own flag/mode, engine path with spaces passed intact, no environment setting changes, repeated no-path build preserves existing source cache, missing source fails before canonical output modification. Implement source resolution, engine identity/input evidence in diagnostic report, and preserve failures/exit codes. Build/profile changes do not install dependencies.
- [ ] Step6: Qt functional RED: actual `installMotionLab(window,{"test"})` produces exactly one page in working mode, repeated call no duplicate; isolated/reference without flag yields no page and flag still creates one. Compile separate test targets if needed; no product runtime test hook. Implement minimal entry condition. No UI layout/controller/worker/timer changes.
- [ ] Step7: Full deployment/build Python suites, expanded actual boundary matrix, full both-mode standalone Motion Lab builds+CTest serial with actual QtTest totals and owned captures. Working entry mode must be exercised separately. Static candidate configure/link behavior tested without touching canonical outputs; full product app build belongs to Task2. All failures/warnings/skips named; no raw overwrite.
- [ ] Step8: Self-review and task report with functional RED/GREEN, exact commands/counts, files, missing scenarios and candidate commands. ROOT ONLY commit then independent task review and scoped fixes. Worker never stages/commits/pushes or spawns agents.

### Task 2: Final review, clean candidate and recoverable working installation

**Files:** root docs/PART26L_OWN_WORKING_RELEASE_REPORT.md, STATE, this plan/ledger;
host docs/build/current-release.md, output-layout.md and dated acceptance record.
Scratch scripts/evidence under engine out/part26l, host build/checks/part26l-*.

**Interfaces:** consumes reviewed Task1 flags/presets/install rules, exact26-file
own package, own entry tests and recorded build commands. Produces canonical
verified EXE/PDB plus complete previous backup. No additional product features.

- [ ] Step1: Whole-stage independent review before promotion; at most one combined final fix and one scoped re-review. Every deferred/declined finding dispositioned. Freeze clean host+engine commits and source hashes.
- [ ] Step2: Validate exact canonical Release/symbol paths/no reparse and no running canonical EXE. Copy entire old package and matching PDB to new build/checks/part26l-promotion-<unique>/backup; hash-verify all copied files. Never terminate user processes. Preserve old files before any canonical writes.
- [ ] Step3: Fresh own-static candidate build in build/cmake/part26l-own-release, configured canonical prefix and engine absolute source, actual existing /MT SDK, no install. Build app+smoke, serial CTest. Stage Runtime to new evidence/candidate, verify exact notices bytes against sources, metadata identifies actual frozen engine, release readiness/import audit, actual no-reference link closure and matching PDB via existing symchk tool. Confirm canonical old pair unchanged throughout build/staging. Preserve raw paths; no unverified tool install.
- [ ] Step4: After candidate verification copy/install exactly approved package and PDB to working locations. Verify actual copied hashes, repeat release readiness and symbol match. On failure restore verified backup contents/pair (only validated exact paths), retain evidence and report. No broad recursive deletion or package cleanup outside exact inventory. Perform a sequential owned GUI smoke only through existing accepted test harness if its settings/process boundaries are safe; otherwise state native acceptance limitation rather than changing settings.
- [ ] Step5: Verify engine K product inputs unchanged; no needless full engine suite for packaging-only change. Record fresh host test totals, app/static smoke/import/notice/symbol/backup outcomes, actual before/after hashes and build identities. Update current-release/output-layout/STATE/ledger, scoped commits and ordinary pushes after remote-base checks. Keep all raw/SDD and no automation. Continue separate design for chosen full primitive_geometry coverage; don't call installation feature completion.

## Self-review and approval

Controller-approved under delegated authority. Two tasks share one concrete
package contract; Task1 code/tests are independently reviewable before Task2
changes the user's working files. Every review-focus risk has a named step.
No public license is selected, no reference install exception is introduced.
Execution: subagent-driven, one writer, independent task and whole reviews,
root-only Git and first promotion. Existing main checkouts chosen by user.
