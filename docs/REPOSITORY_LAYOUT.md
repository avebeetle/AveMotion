# AveMotion repository layout

Approved scope: the user's 2026-10-08 rename/cleanup request. This is a naming
migration, not a new animation feature or a distribution/license decision.

The target local source location is `C:/Users/USER/Desktop/AveMotion`.
Until the directory relocation succeeds, the working checkout and active build
paths remain `C:/Users/USER/Desktop/AveMotion-CorpusLab-Part24`.
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
- The desired GitHub name is `avebeetle/AveMotion`. Change `origin` only after
  the authenticated repository rename succeeds and repository identity is
  verified; retain the working old URL if authentication is unavailable.
- No dependency installation, Windows settings change, UI behavior change,
  public release, force-push, or history rewrite is part of this migration.

## Recoverable generated-file cleanup

After successful relocation, retain old engine `out/build` under
`out/archive/2026-10-08-rename/engine-build` and the host's old default
`build/cmake/own-static` under `build/checks/2026-10-08-rename/archive/own-static`.
A fresh default tree restores routine build routing; do not archive the working
cache before relocation succeeds. Archived caches are evidence, not reusable
builds. The generated `scripts/__pycache__` can be retained separately under
`out/archive/2026-10-08-rename/scripts-pycache` without affecting current builds.
All other raw directories stay intact.

Current execution status and verification are in
[STATE.md](superpowers/STATE.md); the bounded
[migration plan](superpowers/plans/2026-10-08-avemotion-name-migration.md)
records the checks. Paths above are the migration's target layout; consult STATE
for completion or a filesystem/authentication blocker.
