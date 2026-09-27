# Part26I — own ellipse playback

Status: in progress. Both task reviews accepted; whole-stage/frozen final gates
are not yet complete. Do not interpret this checkpoint as final acceptance.

## Scope

Reuse first-party playback control without a second scheduler. Private
PlaybackControl is extracted from Runtime; reference-enabled Instance delegates
while its no-reference guards and frame-mapping variants stay intact. A private
own playback wrapper connects explicit host time to the accepted H scene stream.
No Player, public loader, renderer or Avelabs UI switch is included.

The pinned Telegram source is
TelegramMessenger/rlottie67f103bc8b625f2a4a9e94f1d8c7bd84c5a08d1d,
local third_party/rlottie/telegram/source/src/lottie/lottiemodel.h:613-623.
Duration is float division of (N-1) by float frame rate, then widened to double;
normalized frame mapping truncates double position*(N-1). This is a concrete
source-backed compatibility decision, not a claim of Telegram performance.
Further architectural decisions will likewise inspect their relevant source.

## Accepted checkpoints

- Design3725291, plan98c2587, Task1 product78e7c39, accepted handoff1f0f953.
- Task1 independent spec/quality review Approved; no Critical/Important.
  Full none39/39,4.68s;Telegram87/87,99.19s;Samsung targeted4/4,0.46s.
  Root focused Telegram4/4,0.37s. Pure snapshots, success-only Holding,
  revision/anchor transitions and numeric boundaries have literal tests.
- Task1 compiling-stub RED retained. Initial misplaced test registration was a
  setup failure, not RED. One full CTest invocation lacked VS compiler PATH and
  failed only nested legacy configuration; full VS rerun passed without changing
  a test or product policy. Raw failed/successful logs are retained.
- Samsung vendor C4251 warning retained; no vendor edits or full Samsung claim.
- Task2 product9f56f29, independent spec/quality Approved. Full none40/40,4.89s;
  Telegram89/89,102.33s;Samsung own1/1. Root fresh2/2,2.50s confirms840 scene/plan
  comparisons (15existing assets,4viewports,14emissions). Shared-control snapshots
  prove wiring only; literal expectations independently prove timing behavior.
  Initial missing test include and wrong expected trace count retained, without
  product-policy changes. Initial Telegram vendor C4251 warning also retained.

## Pending final proof

Whole-stage review and frozen-HEAD full none/Telegram/Win32
preview gates, Samsung targeted old/new playback, actual include traces and
manual link/install/host checks remain required. Raw evidence belongs to
out/part26i, task reviews and recovery ledger to this plan's ignored SDD directory.

## Decisions and remaining boundary

The durable ledger records three decisions with costs: share the existing
private control plus thin wrapper (future scheduler seam may need revision);
preserve exact Telegram arithmetic and existing edge policies (legacy extreme
quirks remain for a separately justified fix); direct main/root-owned Git and
retained evidence (less branch isolation and disk cost).

This remains a constrained own ellipse path, not complete Lottie/SVG, a general
public own loader or finished host playback. H own-only identity/cache domains,
single-writer objects, allocator lifetime and caller forget/reset still apply.
No measured speedup, zero-allocation, hardware GPU or TSan claim is made.
Next separately design minimal scheduling/host adaptation using the existing
Player and Avelabs event loop, then isolated static host acceptance. Do not add
a second timer/worker or replace the user's accepted UI executable in this stage.
