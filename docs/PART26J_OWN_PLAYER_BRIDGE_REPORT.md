# Part26J — own playback in the existing Player

## Outcome

Own ellipse playback is registered in the existing Player through a private
owning source binding and two typed Rendering-private functions. Runtime and
own sources share the actual scheduler, controls and callback coalescing, not
a copied scheduling loop. Own frames have genuine Player identity and empty
Runtime identity. Own graphics resources remain in a separate domain.

This is the scheduling prerequisite for an own visual Avelabs pilot. It is not
full Lottie/SVG support, a public own loader, a Qt display implementation or a
performance result. The current Avelabs Motion Lab output is still reference
CPU -> QImage. Existing public Runtime behavior remains reference-backed.

## Commits and review

Base: `084d76079c4187c43ac7081ce8c8a0708e4a2cde`.
Final tested product: `20d9a8f66314a07da417a39e81dd5d6d33dbfe0e`.

- `5489760`, `3558643`: written design, plan and controller approval under user delegation.
- `6e9c769`: private Player source binding; `a881b68`: isolated remove/repaint test.
- `28e0c6e`: accepted Task1 handoff.
- `6aaeb36`: typed own registration; `253cc25`: two-instance planner retirement proof.
- `70ca815`: accepted Task2 handoff.
- `20d9a8f`: live Runtime identity preservation plus final coverage and contract clarification.

Both tasks had compiling-stub functional RED before implementation, GREEN and
independent spec/quality reviews. Each task review exposed one test isolation
gap, corrected in one scoped test-only round. Whole-stage review found a real
compatibility defect: caching the Runtime handle at registration was wrong if
the caller move-assigned a replacement Instance into the same retained object.
A focused test reproduced it before the narrow production fix restored the
pre-stage live handle read. The single combined final wave also added snapshot
comparisons on empty early/paused ticks. Exactly one scoped final re-review
accepted both findings, with no new breakage. No open product finding remains.

## Frozen final validation

All final commands use the product SHA above and the retained instrument under
`out/part26j/gate-evidence.ps1`, which initializes MSVC in its process, configures,
incrementally builds, runs CTest and preserves raw logs, full LastTest, JUnit,
source/configuration hashes, instrument hashes and generated graph identity.
These are not clean rebuilds or benchmarks.

| Preset/scope | Final result | Time | Attempt |
| --- | --- | --- | --- |
| windows-msvc-direct2d, full, reference=none | 42/42, zero skipped/failed | 4.86s | 002 |
| windows-msvc-telegram-debug, full | 92/92, zero skipped/failed | 107.53s | 001 |
| windows-msvc-win32-preview, full | 87/87, zero skipped/failed | 112.36s | 001 |
| windows-msvc-samsung-debug, targeted | 7/7, zero skipped/failed | 0.55s | 001 |

The preview gate includes capture/WARP/device recreation and the preview selftest.
Unchanged H/I checks still pass:7920 semantic comparisons/12 mutation witnesses,
840 ordinary playback comparisons and180 pixel cases (160 visible,20 empty).
These are retained regression checks, not new feature/performance claims.

Reproduction commands from the repository root:

```powershell
$jTag = '20d9a8f66314a07da417a39e81dd5d6d33dbfe0e'
& ./out/part26j/gate-evidence.ps1 -Mode Gate -Preset windows-msvc-direct2d -EvidenceTag $jTag
& ./out/part26j/gate-evidence.ps1 -Mode Gate -Preset windows-msvc-telegram-debug -EvidenceTag $jTag
& ./out/part26j/gate-evidence.ps1 -Mode Gate -Preset windows-msvc-win32-preview -EvidenceTag $jTag
& ./out/part26j/gate-evidence.ps1 -Mode Samsung -EvidenceTag $jTag
& ./out/part26j/trace-includes.ps1 -EvidenceTag $jTag
```

Each run creates a new attempt directory and refuses evidence overwrites. Running
these historical commands after a documentation seal requires using the current
full HEAD and retaining the source-identity comparison, not silently changing
the old attempt. The final report is not itself a portable replacement for the
retained local instruments/raw artifacts.

## Boundaries and limitations

Source and generated no-reference product/export graphs agree on
Rendering -> Player -> Runtime, with no reverse cycle. The installed header
change is only an opaque friend declaration; private source/adapter headers
remain under src and are not installed. The static export uses a link-only
Player dependency; no private include directory leaks into its interface.
Root verified all21 source/generated snapshot hashes in the read-only link audit.

The own-player/product closure has no rlottie or Reference library. A distinct
offline test deliberately links the no-rlottie Reference wrapper; do not describe
the entire test build as wrapper-free. An old install_manifest.txt predates this
stage and is not current install proof. This stage inspects the current generated
install/export graph; it does not execute an installed consumer or a /MT host.
Reference-linked installation remains prohibited.

Actual MSVC include traces for Player.cpp and OwnNativeEllipsePlayer.cpp on
none/Telegram each show154 and115 unique headers respectively, zero unclassified
or vendor/reference headers. The original objects and generated graphs remained
unchanged by syntax-only tracing. Both vendor trees/committed corpus and16 TGS
assets pass integrity verification. All accepted manifests agree on932 source/
configuration inputs and8 instrument hashes; root rehashed all recorded raw
evidence and found no changes. The documentation-only seal preserves these
identities; its final published SHA is recorded in the SDD checkpoint.

Avelabs remained clean at `712d454a7c5c175ad59a3ca547c6b22ce8392da1`, its out
directory absent, and accepted build/Release/AvelabsUI.exe SHA256 unchanged:
`C92F26EE8FEB4FF03F6DC7F4AFFF4B11FB48142743D1EDCCB4E213AAA81F82C2`.
No host build/GUI, dependency installation, Windows setting, automation, vendor,
fixture, golden, graphics ownership, threading or fallback-policy change.

Historical Samsung vendor/MSVC warnings remain in raw logs, without suppression.
The known full Samsung Polystar failures are outside the targeted claim. No
performance/speedup, zero-allocation, TSan, hardware GPU, UI/DPI or complete
sticker compatibility claim is made.

## Evidence and process limitations

Retained task reports/reviews: `.superpowers/sdd/2026-09-27-own-player-bridge/`;
raw build/test/provenance/include artifacts: `out/part26j/`.
Initial task CMake/test compile mistakes preceded the actual functional REDs;
they are preserved and not called behavioral evidence.

Final none001 passed42/42 but its new report collector falsely rejected a fresh
LastTest timestamp by comparing local DateTime ticks with UTC. The raw attempt,
failure and recovered full LastTest remain. Only the ignored instrument changed:
DateTimeOffset normalizes the comparison to UTC without changing its two-second
tolerance. Six actual-predicate cases exercise fresh/stale/threshold values for
Z/+03 offsets; the author reported functional RED and root reran GREEN6/6 with
saved output. Accepted none002 and subsequent final gates use the corrected
instrument hashes. No product test was weakened and no extra product fix wave
was performed.

## Decisions and next step

The four delegated decisions and their costs are recorded in the durable ledger:
retain one private-table Player seam (an indirect call per operation); schedule
before Qt output (no own UI preview in this checkpoint); direct main and retain
raw/SDD (no branch isolation, retained disk usage); project Runtime identity live
(the same per-frame handle read as before, registration metadata documented).

Next separately design the smallest internal own preparation/render host bridge,
then isolated static Avelabs acceptance. A possible bounded pilot is own scene/
plan -> offscreen Direct2D/WARP -> QImage readback, explicitly not end-to-end GPU
presentation. Preserve the accepted UI Release, one existing worker/timer,
serial ownership and explicit cache retirement. Do not repeat completed J,
broaden feature coverage silently, or revive an automation merely to fill time.
