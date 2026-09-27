# Part26J durable ledger

Spec5489760, plan docs/superpowers/plans/2026-09-27-own-player-bridge.md.
Base084d760; controller-approved under explicit user delegation, not direct user
review of these documents. Execution selected SDD, root-only Git, one writer.

## Preflight

Main normal checkout (.git==common.git), user explicitly chose main; no worktree.
Fresh unchanged no-ref baseline40/40,5.17s inside VS (tool transcript). No running
prior writer; two read-only scheduler/host inventories completed without edits.
Host clean712d454, acceptedRelease hash unchanged, no host changes authorized by
this bounded J stage. Root read Player and current MotionWorker/build seam.
Pinned Telegram lib_lottie source revisited and Qt6.10 thread contract read.

| Task/pair | Shared interface or consistency | Disposition |
| --- | --- | --- |
| Task1 self | private source, real scheduler, literal test source | test-only concrete source uses existing control; no duplicate algorithm |
| Task2 self | own preparation/playback -> typed add/lookup -> actual Player | own Runtime fields null/invalid; evaluate separately |
| Task1/2 | PlayerSourceOps/PlayerSource/PlayerSourceAccess + CMake | exact signatures pinned; sequential source owners; Rendering -> Player only |
| Both/final | include/private friend/export closure and source ownership | actual traces/links and frozen gates; public facade remains Runtime-only |

Ruling: Keep the existing Player with a private source table and two typed own registration functions — smaller than another scheduler/facade registry — cost if wrong is revising this internal seam, plus one indirect call per source operation.

Ruling: Complete the scheduler prerequisite before the visual Qt bridge — current QImage path is reference CPU and cannot display own scenes directly — cost is that this checkpoint does not yet provide an own visual UI preview.

Ruling: Follow explicit main/root-Git workflow and retain raw/SDD evidence — preserves user workflow/recoverability — cost is no branch isolation and retained disk usage.

## Progress

Design committed5489760; detailed plan self-reviewed, tests cover all five focus
risks. Next Task1 writer, task review, Task2, whole-stage review and final gates.
No automation, dependencies, Windows settings or Avelabs changes.
