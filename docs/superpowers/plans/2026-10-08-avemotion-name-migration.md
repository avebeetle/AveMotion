# AveMotion Name Migration Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development. Steps use checkbox syntax for tracking.

**Goal:** Unify active engine naming and source paths as AveMotion without losing evidence or replacing the installed Avelabs package.

**Architecture:** Relocate the primary checkout, archive path-bound generated trees, and configure fresh builds. Batch the small active documentation/test path edits into one reviewed change. GitHub authentication is an independent gate, not authority to access credentials.

**Tech Stack:** PowerShell, Git, CMake 3.26.4, MSVC 2022, existing Qt 6.10.0 /MT, Python 3.9.

**Spec:** `docs/REPOSITORY_LAYOUT.md`, based on the user's explicit naming-migration request; ordinary plan approval/execution/main commits/push are delegated.

## Global Constraints

- Preserve Git history, vendor sources/patches, licenses/notices, fixtures, goldens, private artwork, and raw results under `out/`.
- Update active source-path examples and build tests, not dated evidence or historical ZIP names.
- Do not edit generated CMake caches to simulate relocation.
- Change `origin` only after the authenticated repository rename succeeds and repository identity is verified.
- No dependency installation, Windows settings change, UI behavior change, public release, force-push, or history rewrite is part of this migration.
- Work in the existing main checkouts, as explicitly chosen by the user. No extra worktree or optional refactor.

## Review Focus

Target already exists; live path-bound process; stale default host build route;
accidental rewrite of historical evidence; installed package replacement.
Task 1's relocation guards and before/after hashes cover these conditions.

### Task 1: Naming, relocation and fresh build acceptance

**Files:** Modify engine `README.md`, `docs/superpowers/STATE.md`; host
`tests/build/test_own_release_build.py`, `tests/motionlab/test_build_boundary.py`,
`docs/build/own-motion-lab.md`, `docs/build/diagnostic-static.md`,
`docs/build/current-release.md`. Create only this plan and `docs/REPOSITORY_LAYOUT.md`.

**Interfaces:** New engine source `C:/Users/USER/Desktop/AveMotion`; unchanged
host `AVELABS_AVEMOTION_SOURCE_DIR` cache option, build wrappers and package API.

- [ ] Record clean E9bd2004/H1e286e4, origin, installed Release+symbols hashes,
  target absence, root/link state, stale-worktree dry run and process inventory.
- [ ] Batch active path edits to the exact new directory. Rename README title
  to `AveMotion` and distinguish current bounded own path from reference Runtime;
  retain the corpus-lab instructions and historical reports.
- [ ] Before moving, stop dispatching writers and verify exact resolved source,
  destination, parent, no reparse roots and no relevant live build process.
  Move old checkout to new with native PowerShell. On an OS lock, preserve the
  source and stop only relocation; do not kill foreign processes or copy/delete.
  Restore only this task's not-yet-valid active source-path edits before a
  partial handoff; retain the README rebrand and honest pending route notes.
- [ ] Retain path-bound engine out/build, generated pycache and default host
  own-static under the exact archive paths in the spec. No recursive delete.
  Preserve every other raw output directory. Prune only the proven nonexistent
  linked-worktree registration after an unchanged dry run.
- [ ] Configure/build/test the engine fresh `windows-msvc-direct2d` preset
  with MSVC x64; record actual CTest count. Configure/build fresh host own-static
  with explicit new engine path and dependencies installation OFF; build only
  `AvelabsUI avemotion_engine_smoke`, run the smoke, do not install/launch.
- [ ] Run host `test_build_diagnostic.py` and real `test_own_release_build.py`
  with fresh owned output roots. Record failures before repair; no test weakening.
  Confirm no-reference link/imports and unchanged installed package/PDB hashes.
- [ ] If user restores GitHub auth, inspect existing repo identity/default
  branch/visibility and target availability, rename only its name, re-read same
  identity, then update origin. Otherwise record that this part remains pending.
- [ ] Independently review task diff and evidence, update STATE/ledger with
  actual results and limitations, scoped commits and ordinary pushes after
  remote comparison. No automatic package promotion in a rename task.

Controller owns filesystem/GitHub actions and validation; one product writer
owns tracked metadata edits. This is configuration relocation, not a production
feature/bugfix: no new C++ code or artificial behavior test is needed. Existing
real configure/build/run gates prove the changed path, not string-only tests.
