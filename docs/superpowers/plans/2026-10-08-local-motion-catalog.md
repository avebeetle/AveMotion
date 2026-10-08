# Local motion catalog implementation plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Make a provenance-declared local animation list available on the existing Voices page through the current worker.

**Architecture:** A bounded Qt metadata helper reads one app-local manifest. The current load intent carries optional expected digest/root, and its worker checks the single input buffer before preparing it. The page owns selection/restoration policy; no engine, renderer, scheduler or network subsystem changes.

**Tech Stack:** Windows x64, C++20, Qt 6.10.0 Widgets/Core/Test, MSVC, existing static `/MT` and standalone dynamic `/MD` builds.

**Spec:** `docs/superpowers/specs/2026-10-08-local-motion-catalog-design.md`.

## Global Constraints

- Engine production code is not changed. Own software WARP and public fallback policy remain unchanged.
- Maximum manifest size 65536 bytes, maximum entries 64; selected asset keeps the existing 2 MiB limit.
- Version1, validated IDs/default/digest/provenance, constrained relative JSON/TGS paths, useScope local-evaluation-only.
- File/digest/root coalesce as one request; hash and prepare the same QByteArray in the existing worker.
- Effective visibility gates metadata/default selection and selected preparation; no mass preparation or hide/show retries.
- Successful catalog/manual choices switch mutually exclusive restoration keys; failures preserve the last accepted choice.
- Package remains exactly26files; notices/PDB/source provenance preserved. No artwork/catalog in Git or package.
- Work directly in main, scoped commits and ordinary push only after checks/review. No force/history rewrite/Actions/dependency installs/Windows changes.
- One product writer; sequential GUI. Do not terminate user apps or substitute QtTest for native acceptance/performance.

## Review Focus

1. File+digest+root replacement during mailbox coalescing must never apply a stale digest to manual bytes (Task1 test).
2. Sibling-prefix/link escape must be rejected before preparing selected content (Task1 test).
3. Failed or superseded catalog choice must not erase accepted manual/catalog preferences (Task2 test).
4. Preexisting manual preferences and explicit catalog success must not bypass digest on recreation (Task2 test).
5. First hidden/minimized page and cancellation before visibility must not start an unexpected asset (Task2 test).

## Task 1: bounded metadata and verified worker requests

**Files (H = D:/rvc/c++/DragonianVoice/Avelabs-UI):**
Create `H/src/app/motionlab/MotionCatalog.h/.cpp`, `H/tests/motionlab/tst_motion_catalog.cpp`.
Modify MotionTypes.h, MotionMailbox.cpp, MotionController.h/.cpp, MotionWorker.h/.cpp,
H/CMakeLists.txt, H/tests/motionlab/CMakeLists.txt, H/tests/motionlab/tst_motion_worker.cpp.

**Interfaces:**
Produce `MotionCatalogEntry` with QString id/title/file/source/license/useScope and raw QByteArray sha256;
`MotionCatalog` with QString root/defaultId and QVector<MotionCatalogEntry> entries;
`MotionCatalogResult` with MotionCatalog catalog, QString error, bool present, explicit bool conversion (present and no error).
Functions: `readMotionCatalog(const QString&)`, `defaultMotionCatalogPath()` and
`resolveMotionCatalogAsset(const QString& root, const QString& path, QString& error)` returning a contained canonical absolute file path.
Extend `MotionController::loadFile(QString file, QByteArray expectedSha256 = {}, QString catalogRoot = {})`;
ControlIntent fields expectedSha256/catalogRoot adjacent to file; worker load receives both.
The resolver accepts the full selected path (parser file is relative); reject invalid root/selection or escaping canonical path.

- [ ] Write functional tests before production logic. A minimal compile-only stub/unused parameter plumbing is allowed for new interfaces; retain that RED baseline before the real implementation.
- [ ] Metadata tests: valid ordered entries/raw digest/default; file absence; malformed/non-object/version1.1; bytes65536 accepted as whitespace-padded valid JSON and65537 rejected;64entries accepted/65rejected; duplicate IDs/unknown default; lengths; missing fields and scope; raw path traversal/ADS/device/reserved/trailing-dot/space rows. Tests assert rejection/error and no published entries.
- [ ] Worker tests: existing valid bytes+matching SHA succeed, wrong SHA fails without replacing last asset, malformed constraint rejects, inside-root file succeeds, sibling-prefix/escaping selected path rejects; manual request remains unconstrained. Mailbox tuple replacement clears old root/digest and unrelated controls preserve it.
- [ ] Run focused catalog and new worker cases, capture real assertions failing on baseline, not mere compile failure. Use isolated build H/build/cmake/local-catalog-own-tests-001; raw H/build/checks/2026-10-08-local-catalog-001/task1.
- [ ] Implement small QtCore helper and same-buffer SHA verification before prepare/install; use existing load failure semantics and generation filters. Do not add a new thread or new fallback. No filesystem sandbox claim.
- [ ] Build/run focused GREEN, then complete own CTest once; also fresh reference standalone interface-compatibility full gate, separate H/build/cmake/local-catalog-reference-tests-001. No UI/package promotion. Disclose skips/warnings.
- [ ] Self-review and report exact RED/GREEN/changed paths; controller independently reviews, updates STATE/ledger, scoped commits and ordinary push. Commit subject `feat: add bounded local catalog and verified motion loads`.

## Task 2: visible selector and success-only restoration

**Files:** H/src/app/motionlab/MotionLabPage.h/.cpp; H/tests/motionlab/tst_motion_page.cpp.
Consumes Task1 helper/controller interface. Preserve `MotionLabPage(QWidget* parent=nullptr)`;
add `MotionLabPage(QString manifestPath, QWidget* parent=nullptr)` (explicit empty path disables catalog).
Selector objectName motionLabCatalog; separate metadata error QLabel motionLabCatalogError;
provenance tooltip plain text. Keep current controls/page min180/max270/canvas badge.

- [ ] Add functional RED tests on Task1 production: first visible valid catalog/default autoload and autoplay; hidden/minimized no load; list order/placeholder; only selected file loaded.
- [ ] Saved manual choice wins preexisting dual keys. Accepted explicit catalog clears manual key and saves ID; recreation retains hash enforcement. Accepted manual clears ID; failed/superseded replacement changes neither. Unknown saved ID fails once, no default fallback.
- [ ] Invalid/missing catalog remains recoverable by picker; errors use separate label and don't replace active playback. Metadata list population is signal-blocked. Explicit selection loads paused; auto restore plays, Pause/Stop/Seek/slider press cancels delayed autoplay, hidden pending selection is last-wins.
- [ ] Isolate every page's catalog in a temporary/explicit-empty path, including existing test page creation. Add real same-drive relative manual path test, assert QDir::isRelativePath(input) before load; persistence must be absolute. Do not recreate MainWindow or weaken original tests.
- [ ] Implement smallest page integration via existing load/autoplay/visibility callbacks; no catalogs automatically registered from manual import, no hot reload/watcher.
- [ ] Focused GREEN then full own/reference page suites once; original unchanged a-2/a-3 focused loaded/playback page runs, hash-bound temporary manifests, separate logs. Skips not invoked are disclosed, not silent substitutes.
- [ ] Self-review/report, independent task review, STATE/ledger, scoped commit/push. Commit subject `feat: select and restore local catalog animations on Voices`.

## Task 3: actual static delivery and local development seed

**Files:** documentation/report and ignored local evidence/scripts only; no new product feature.
Consume reviewed Task2 product commit and unchanged engine production lineage.

- [ ] Freeze clean source SHAs. Fresh own-static actual `/MT` AvelabsUI/smoke build from canonical E root, install dependencies OFF; exact generated identity/CRT/link/import/smoke evidence. No need to repeat old engine lifecycle gates for unchanged engine production.
- [ ] Stage exactly26files and paired PDB. Full canonical backup; compare notices to sources; package/import/readiness/SymChk gates, all raw outputs bound to hashes.
- [ ] Prepare app-local seed in a new directory with unchanged original a-2/a-3, sourceb8951c8 paths/hashes and explicit artwork license unresolved/local-evaluation-only. Manifest uses version1/defaultId/entries matching Task1. Never overwrite a conflicting existing manifest or seed; publish manifest last. This is machine-local, not shipping artwork or clearing rights.
- [ ] Root audits publication/path guards; do not seed from a general network download or commit sticker bytes. Confirm actual Qt default path through the current app identity/path API without settings changes.
- [ ] Independent whole-stage review on source/test/delivery/seed/docs evidence; triage all deferred findings, preserve exact native/performance limits.
- [ ] Recheck canonical apps exited (user said both Quit; observation still required), guarded promotion, post-install package/PDB verification. If a process is running stop only promotion, keep safe source results.
- [ ] Native attempt through Computer Use on own EXE, fresh windows only. Verify list/default/restoration/transport and both originals if tool works; stop capture/input failures without blind actions. No performance claim from this check.
- [ ] Update current STATE/current-release/report/ledger; test links/diff; scoped documentation commits, ordinary push, clean trees and exact remote receipts. Preserve backups/raw evidence; do not start unrelated corpus/backend work mid-handoff.

## Controller approval and preflight

Spec and plan self-reviewed: every requirement belongs to Task1/2/3; interfaces
are consistent, test values copied above, no unresolved placeholders. Controller
chooses sequential subagent-driven execution under the user's delegation. Product
writers cannot commit/push/promote; controller integrates reviewed changes.
The overlapping files/interfaces and task consistency table lives in this plan's
SDD ledger. This plan does not wait for routine confirmations already delegated.
