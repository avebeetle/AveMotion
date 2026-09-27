# Part26I durable ledger

Plan: docs/superpowers/plans/2026-09-27-own-ellipse-playback.md
Spec: docs/superpowers/specs/2026-09-27-own-ellipse-playback-design.md
Base345f1b6; controller-approved under explicit user delegation. Root owns Git.

## Preflight

Clean main, remote exact345f1b6 before design. H agents completed; two read-only
focused inventories only, no duplicate product writer. Fresh none baseline38/38,
5.07s under VsDevCmd. Baseline output is tool transcript, not a retained unique
raw file; task/final raw evidence will use unique files. Host read-only checkpoint
clean712d454, out absent, accepted EXE hash
C92F26EE8FEB4FF03F6DC7F4AFFF4B11FB48142743D1EDCCB4E213AAA81F82C2.

| Scope | Producer / consumer or self-consistency | Finding |
| --- | --- | --- |
| Task1 | control literals -> extracted helpers -> unchanged Instance | preserve no-ref guards; no behavior correction hidden in extraction |
| Task2 | sealed preparation -> own stream + shared control -> snapshot/scene | independent literals required because Instance shares helper |
| Task1/2 | PlaybackControl.hpp functions consumed by wrapper; shared CMake | sequential ownership; exact function contracts; no scheduler duplication |
| Both/final | new private sources -> none/reference links/install/include | final actual traces and full graph gates; no public headers |

Ruling: Extract existing private playback control and add only a thin own wrapper — avoids duplicate algorithms and leaves Player/host untouched — cost if wrong is reworking this seam during scheduler integration, not an extra parallel scheduler.

Ruling: Preserve exact Telegram float duration arithmetic and existing Runtime edge policies — behavior compatibility before optimization — cost is retaining legacy extreme-value quirks for a separately justified fix rather than silently improving them here.

Ruling: Continue direct main with root-only Git and retain raw/SDD evidence — explicit user workflow and recoverability — cost is less branch isolation and retained disk usage.

## Progress

Design3725291 committed; plan self-review complete. Next Task1 through TDD and
independent task review. User's request to consult Telegram periodically becomes
a source-inspection checkpoint at architecture decisions, not a new automation.
Local pinned Telegram source already establishes duration/mapping. Avelabs host
inventory confirms existing clock/QTimer/worker/mailbox boundary; scheduling and
host adaptation remain next design, not extra scope in this control block.
