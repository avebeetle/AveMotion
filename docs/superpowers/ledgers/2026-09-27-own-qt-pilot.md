# Part26K durable ledger

Plan docs/superpowers/plans/2026-09-27-own-qt-pilot.md; spec9170bc5.
Stage bases AveMotion1bab64b / Avelabs712d454. Controller-approved written
spec/plan under explicit delegation; selected SDD, root-only Git, one writer.

## Preflight

Both clean main, ordinary remotes exactly equal bases, .git==common.git,
no active prior writer. Accepted Avelabs EXE hash C92F26EE...82C2 unchanged,
out absent. Read STATE/J spec/ledger, Avelabs ROADMAP/maintenance/current tray.
Independent read-only engine and host inventories under out/part26k-design.

| Task/pair | Contract check | Disposition |
| --- | --- | --- |
| Task1 self | own helper + selector + existing worker/page | single atomic vertical slice, all signatures and limits pinned |
| Task2 self | review/frozen tests/static artifacts/visual evidence | no new product behavior; tests distinguished from native acceptance |
| Task1/2 | mode, targets, own image and diagnostics | exact own-warp/default reference mode; no installed/public engine API |

Ruling: Keep the new glue private in Avelabs, using existing engine headers and one worker — smallest route to own UI pixels — cost if wrong is revising a build-tree-private seam later.

Ruling: Use compile-time own-warp with no rlottie and WARP-to-QImage readback — proves own pixels without changing window composition — cost is CPU readback overhead and no hardware acceleration claim.

Ruling: Preserve the stricter own JSON limit and unsupported subset, with explicit errors — do not expand parser/format scope to make host tests pass — cost is rejection of most real stickers until later feature work.

Ruling: Work on the two existing main checkouts with root-only Git and retain raw/SDD — follows explicit user workflow — cost is no branch isolation and retained disk usage.

## Progress

Design written/self-reviewed/committed9170bc5. Plan self-review complete;
Task1 pending sole writer, functional RED, GREEN, task review. Task2 pending.
No accepted package/dependency/Windows/automation change.

Ruling: Check own app/smoke actual build/link closure, not the mere declaration of avemotion_reference — current engine always declares an EXCLUDE_FROM_ALL wrapper and spec prohibits building/linking it, not its declaration — cost if wrong is a missing transitive dependency detection, addressed by final actual link inspection.
