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
Task1 complete host7e0ec58 + testfixe701e3f, independent task review/fix1 accepted.
Reference CTest3/3(QtTest42pass), own4/4(52pass) before review; renderer10/10
after adding required translucent raw-byte assertion. Boundary31PASS; no skip.
All failed attempts and mutation witnesses retained, not labeled historical RED.
Root confirmed protected Release hash/out unchanged. Task2 now pending.
No accepted package/dependency/Windows/automation change.

Task1: minor (deferred): selected variant persists in cache and prevents an
in-place renderer-mode switch; documented fresh-directory workflow avoids it.
Whole-stage reviewer must triage. Reference67 warning lines and own4 derive from
unchanged vendor/UI sources; no new Motion Lab warning in recorded builds.

Ruling: Check own app/smoke actual build/link closure, not the mere declaration of avemotion_reference — current engine always declares an EXCLUDE_FROM_ALL wrapper and spec prohibits building/linking it, not its declaration — cost if wrong is a missing transitive dependency detection, addressed by final actual link inspection.

Ruling: If no admitted asset can trigger a late render error, allow a private test-access friend to inject an invalid viewport at the existing render boundary — exercises the real refusal and worker latch without a product fail switch or fake pipeline — cost is a private test seam and no claim of injected native device-loss coverage. Written spec addendum governs Task1.

## Task2 acceptance and handoff

Whole-stage independent review complete on host712d454..e701e3f and engine
docs1bab64b..ecac1a5: no Critical/Important. No final product fix or scoped
re-review necessary. Cached mode-switch Minor remains deferred under supported
fresh-tree workflow. All six declined judgments are explicit report limitations;
none substituted for mandatory artifact/platform/protected-state acceptance.

Frozen final suites: engine none42/42,5.18s and preview87/87,106.34s,923 selected
input hashes stable; host reference3/3,6.45s(QtTest42), own4/4,7.13s(QtTest53),
259 selected host inputs stable. Zero failures/skips/disabled; QtTest counts
include init/cleanup. Fresh executions after incremental builds, not clean.
Root viewed two fresh own-page Qt grabs: real moved cyan ellipse and exact badge.
Device/target1/1/readbacks1->2; render6,472,600ns->251,500ns, observations not A/B.

Fresh full static Qt6.10 /MT AvelabsUI + own smoke built; CTest1/1,0.09s;
actual app/smoke graph, link command/read input records, CRT and import tables
accepted by independent read-only artifact audit. No rlottie/Reference actual
build/link/artifact/install scripts; OFF graph still excludes engine. App34 and
smoke32 allowed Windows DLL imports; no Qt/CRT imports. Audit excludes dynamic
LoadLibrary behavior/distribution certification. Both artifact hashes recorded.

Original root scratch static audit used wrong smoke path after successful
build/CTest/app audit. CTest/build output proved nested tests/motionlab/Release
location. Original script/failure preserved; corrected smoke-only audit passed,
517 selected original static inputs and frozen HEADs matched, no rebuild/retest
or product change. Four existing UI warnings and unused overlay configure warning
retained. Not a new product bug and not masked as an all-green original runner.

Protected Release hash C92F26EE...82C2 unchanged; host out absent, no install,
dependency/Windows/vendor/corpus/golden/automation change. Report
docs/PART26K_OWN_QT_PILOT_REPORT.md and host docs/build/own-motion-lab.md contain
actual commands/results/limits. Static EXE available separately, not launched
as a full desktop acceptance. Root-only documentation seal and ordinary pushes
are recorded in SDD. Host docs seal f452922 is ordinary-pushed; host product diff
from frozen e701e3f remains empty. No further implementation in this stage.
