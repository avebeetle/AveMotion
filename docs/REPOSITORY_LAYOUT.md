# AveMotion repository layout

Approved scope: the user's 2026-10-08 rename/cleanup request. This is a naming
migration, not a new animation feature or a distribution/license decision.

The local source location is `C:/Users/USER/Desktop/AveMotion`. The primary Git
checkout and all source/evidence directories have been relocated there. Windows
temporarily retains the old empty root `AveMotion-CorpusLab-Part24`; it is not
another checkout or a build source. Reopen the Codex project at the new path.
The Avelabs host remains `D:/rvc/c++/DragonianVoice/Avelabs-UI`; its working
application remains `build/Release/AvelabsUI.exe`. Source relocation alone does
not replace that installed EXE or its matching PDB.

## Boundaries

- Keep the existing source structure, namespaces and CMake target names.
- Preserve Git history, vendor sources/patches, licenses/notices, fixtures,
  goldens, private artwork, and raw results under `out/`.
- Update active source-path examples and build tests, not dated evidence or
  historical ZIP names. Old absolute paths in reports describe those runs.
- Do not edit generated CMake caches to simulate relocation. Archive superseded
  build trees and configure fresh trees using the new source directory.
- If Windows holds the old root open, a guarded same-volume transfer of its
  immediate children is an alternative. Record every completed atomic child
  move; on failure reverse the ledger without overwriting, copying or deleting
  source data. Report a split state if rollback is blocked. The old empty root
  may remain temporarily locked; this is not an atomic whole-repository rename.
- The desired GitHub name is `avebeetle/AveMotion`. Change `origin` only after
  the authenticated repository rename succeeds and repository identity is
  verified; retain the working old URL if authentication is unavailable.
  The user explicitly deferred the GitHub rename during this migration.
- No dependency installation, Windows settings change, UI behavior change,
  public release, force-push, or history rewrite is part of this migration.

## Recoverable generated-file cleanup

Old engine `out/build` is retained under
`out/archive/2026-10-08-rename/engine-build` and the host's old default
`build/cmake/own-static` under `build/checks/2026-10-08-rename/archive/own-static`.
A fresh default tree restores routine build routing. Archived caches are
evidence, not reusable builds. The generated `scripts/__pycache__` is retained under
`out/archive/2026-10-08-rename/scripts-pycache` without affecting current builds.
All other raw directories stay intact.

Current execution status and verification are in
[STATE.md](superpowers/STATE.md); the bounded
[migration plan](superpowers/plans/2026-10-08-avemotion-name-migration.md)
records the checks. Older reports retain their original absolute source paths:
resolve the old source prefix to the new root, and old `out/build` paths to the
engine-build archive above. GitHub naming remains a separately deferred step.
