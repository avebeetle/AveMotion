# Part26I — own ellipse playback

Status: complete. Final tested source/test HEAD:
`cd55356d9cb24668baaf486f39506d8d88d547de`. Both tasks independently accepted,
one whole-stage review, one tests-only final fix and one scoped re-review done.
Fresh frozen platform and dependency gates below passed before this docs seal.

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

## Final review and frozen proof

Whole-stage review345f1b6..10f29ba: no Critical/Important finding. One Minor proof
gap was accepted: Instance used a first-party mapping calculation, so it was not
the requested direct Telegram mapping oracle. Commitcd55356 changes only that
test: independent cache-disabled ordinary `rlottie::Animation` supplies duration,
rate and `frameAtPos`; its frames now select the fresh ordinary scenes. Nonfinite
input is independently normalized to0 at our boundary. Shared snapshots remain
wiring evidence. Root fresh focused2/2,3.15s; exactly one scoped re-review approved
the fix with no new finding. No new production bug/RED claim is made for this
proof correction. Two vendor C4251 diagnostics remain disclosed, not suppressed.

Sequential final MSVC configure/incremental-build/CTest at frozen cd55356:

| Preset | Scope | Passed | CTest time |
| --- | --- | --- | --- |
| windows-msvc-direct2d | full no-reference | 40/40 | 4.90s |
| windows-msvc-telegram-debug | full Telegram | 89/89 | 104.76s |
| windows-msvc-win32-preview | full, including capture/WARP/device recreation | 84/84 | 109.20s |
| windows-msvc-samsung-debug | shared/old/own playback, mapping, Player only | 5/5 | 0.42s |

All exit0, zero skips/failures; every final attempt001. These are not clean rebuild
or speed measurements. None/TG/Samsung incremental builds had no work; preview
rebuilt61steps and printed the inherited Telegram C4251. Known unrelated full
Samsung scene/plan Polystar failures are not relabeled green by this targeted run.
Both vendors and committed corpus integrity PASS; TGS corpus check16assets PASS.
New matrix840 direct ordinary scene/plan comparisons passed. Unchanged H gates
also passed:7920semantic comparisons/12mutation witnesses and180WARP/CPU cases
(160visible/20empty), including cache/lifetime behavior. Existing D2D capture75
cases and Win32 preview selftest passed; these are not hardware GPU proof.

Exact reproducible commands from the repo root (scripts initialize VsDevCmd in
the same process; raw command arguments/exit codes are retained per attempt):

```powershell
$evidenceTag = 'cd55356d9cb24668baaf486f39506d8d88d547de'
pwsh -NoProfile -File out/part26i/gate-evidence.ps1 -Mode Gate -Preset windows-msvc-direct2d -EvidenceTag $evidenceTag
pwsh -NoProfile -File out/part26i/gate-evidence.ps1 -Mode Gate -Preset windows-msvc-telegram-debug -EvidenceTag $evidenceTag
pwsh -NoProfile -File out/part26i/gate-evidence.ps1 -Mode Gate -Preset windows-msvc-win32-preview -EvidenceTag $evidenceTag
pwsh -NoProfile -File out/part26i/gate-evidence.ps1 -Mode Samsung -EvidenceTag $evidenceTag
pwsh -NoProfile -File out/part26i/trace-includes.ps1 -EvidenceTag $evidenceTag
```

Scripts require the supplied HEAD to be checked out, so later docs-only seals
must use a new HEAD tag for a new run, never reuse the old evidence identity.
Actual nested commands: `cmake --preset PRESET`, `cmake --build --preset PRESET
--parallel 4`, `ctest --preset PRESET --output-on-failure --no-tests=error
--output-junit ABSOLUTE_PATH`; Samsung uses the five specified targets/CTest
names. Integrity: `python scripts/verify_vendor.py --variant all` and
`python scripts/generate_tgs_compatibility_corpus.py --check`.

Evidence: `out/part26i/cd55356d9cb24668baaf486f39506d8d88d547de-PRESET-gate-001`
and `...-include-trace-001`, raw stdout/stderr/JUnit/graphs/identities/manifests;
task/final reviews in `.superpowers/sdd/2026-09-27-own-ellipse-playback`.
Manual disposition: `out/part26i/root-boundary-review.md`. Successful-test JUnit
output can be truncated; full CTest LastTest logs are retained separately in
`out/part26i/cd55356d9cb24668baaf486f39506d8d88d547de-full-ctest-logs-001`.

## Dependency and host boundary

Actual new-source MSVC include traces in none and Telegram: PlaybackControl77
headers (3first-party/44MSVC/30SDK), own wrapper115 (16/67/32);0unknown and no
included vendor/reference/rapidjson header. Telegram Runtime still inherits
vendor search paths, but the new control unit does not include those headers.
Original objects/graphs and926source/config+5instrument hashes stayed unchanged
through every final run. Root inspected regenerated no-ref archive/test links,
exported interfaces, header installation and reference install prohibition: no
new public header/interface or vendor dependency in no-ref playback closure.
No actual install/installed-consumer test or /MT host acceptance is claimed.

Avelabs stayed clean at712d454a7c5c175ad59a3ca547c6b22ce8392da1, UI/out absent.
Accepted Release EXE SHA256 remained
C92F26EE8FEB4FF03F6DC7F4AFFF4B11FB48142743D1EDCCB4E213AAA81F82C2.
No host edits/build/GUI, public Runtime loader, Player, backend, vendor, fixture
or golden changes. No schedule resumed; raw/SDD evidence retained.

Every final reviewer limitation was explicitly dispositioned: fresh in-scope
tests/closure/integrity/host checks above; legacy timing quirks deliberately
preserved; scheduler/host/mixed-namespace/concurrency/cache-eviction guarantees
deferred. Allocation exceptions/counter exhaustion injection remain untested
limits; forwarding existing typed H errors adds no new recovery promise.

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
