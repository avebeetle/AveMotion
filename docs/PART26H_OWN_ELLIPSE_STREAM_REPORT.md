# Part26H — own ellipse scene stream

Status: complete within the approved constrained ellipse scope. Tasks1/2/3 and
the independent whole-stage review are accepted. The sole final Minor was fixed
by comment-only3e655f1 and accepted by one scoped re-review. Final tested product:
`3e655f10fc08e73b8670f6b9e1fabbe05201300e`; the handoff seal is documentation-only.

## Scope

The Rendering-private `OwnNativeEllipseStream` consumes the sealed Part26G own
prepared ellipse owner. It evaluates integer frames, emits owned scenes and feeds
the existing render planner without a production rlottie dependency in this new
path. It is not a public Runtime loader, scheduler, complete Lottie/SVG engine or
host UI integration. The existing public Runtime remains reference-backed.

One linked allocator supplies nonzero nonrecycled IDs; the stream is single-writer.
Different streams may share immutable preparation. Planners/backends in this
stage must be own-only: untagged packed Runtime handles, caller-selected legacy
IDs or a second loaded allocator can collide. Retire cache domains before allocator
unload and explicitly forget/reset history; stream destruction is not eviction.

The narrow legacy-stream edit delegates identical property resolution and
aspect-fit/finiteness operations to a shared private inline helper. It does not
change the legacy certificate, admission, errors, identity or lifecycle policy.

## Accepted task evidence

| Block | Product / review | Evidence |
| --- | --- | --- |
| Own stream and lifecycle | 4e0a957 + d95dea3; both Task1 findings accepted in scoped round1 | none38/38; Telegram own/old/lifecycle3/3; root fresh3/3 |
| Independent ordinary scene/plan differential | 4e1fa45; spec compliant, quality Approved | 7920 comparisons,15 assets,4 viewports plus mixed seeks;12 mutations rejected; full Telegram86/86; root fresh differential1/1 |
| WARP/CPU pixels and retained backend life | de1be3e; spec compliant, quality Approved | 180/180 exact WARP and CPU-policy cases; retained same-stream inactive replay; root fresh old/new capture2/2 |

Task1 covers owned aliases/snapshots after source release, four viewports,
visibility boundaries, clamping, invalid-viewport success history, reverse/repeated
access, JSON/TGS, static sharing, source isolation, shared-planner A/B interleave,
stale/equal/forget behavior and four concurrent distinct-stream workers. Production
counter helpers explicitly test the last valid uint64 value and no wrap. This is
functional concurrency coverage, not TSan or a whole-program race proof.

Task1's four literal scene fingerprints were derived independently from explicit
authored scalar/path/paint/layer fields and the FNV byte layout, without importing
or executing AveMotion as the expected-value generator. Scratch derivation and
its successful output are retained in `out/part26h/task-1/fix-round1`.

Task2 independently parses/samples the unchanged ordinary oracle:12495 direct
samples and150 parses. Only approved identity/display-role/schema metadata in
test copies is normalized. Each original's ownership, roles, resource keys and
fingerprints are validated first; original scenes feed separate planners. The
unchanged exact comparators retain semantic values, revisions, updates and bounds.
Their inherited ordinary-history `upstreamChangeBits` omission is unchanged.
61 own-only emissions changed no live Runtime reference counters; this is not a
substitute for the final reference=none link/include closure.

Task3 uses six existing inputs, six frames and five logical/physical/DPI capture
profiles:160 visible and20 empty controls. All180 original own/ordinary WARP
images match exact BGRA bytes; the separate ordinary CPU raster stays within
the unchanged PixelComparisonPolicy (observed minimum active IoU0.923077,
maximum active-region mean absolute difference2.603750). These are image metrics,
not timing measurements. The unset-draw RED failed all160 visible cases.

The own-only backend test observes A/B separate animated slots, A replay hits,
static sharing, newer-frame replacement, same-stream visible10/inactive20/
retained10 replay, forgetting/rebuilding and external owner destruction. Retained
pixels also survive backend clear, a new WARP surface/device and domain generation
advance. Geometry hit/creation/reset counters are recorded, not a fictional
paint-cache counter. The old capture test passed after the mechanical test helper
extraction. Root's last old/new capture pair passed2/2,5.01s after the final
same-stream replay assertion; the frozen full suite below supersedes that check.

## Final gates

All at frozen3e655f1, sequential, with raw commands/stdout/stderr/JUnit, hashes and
success manifests retained under `out/part26h/3e655f10fc08e73b8670f6b9e1fabbe05201300e-*`.
All attempts001; no failed final attempt or skipped test.

| Preset / gate | Result |
| --- | --- |
| windows-msvc-direct2d (reference=none) | configure/build PASS;38/38,5.13s |
| windows-msvc-telegram-debug | configure/build PASS;86/86,99.07s |
| windows-msvc-win32-preview | configure/build PASS;81/81,108.21s; capture/WARP/device-domain recreation and preview self-test included |
| windows-msvc-samsung-debug | configure/listing only; no full execution claim |
| Vendor / TGS integrity | both vendor trees and committed corpus PASS;16TGS assets PASS |
| Actual OwnNativeEllipseStream compiler trace | all3built presets PASS;119headers each:19first-party,68MSVC,32WindowsSDK,0unknown |
| Private graph/install closure | script and root manual review PASS;919source/config inputs and6instruments stable |

Reproducible commands from the repository root (wrappers initialize installed
VS2022 x64; no dependency installation):

```powershell
$hTag = '3e655f10fc08e73b8670f6b9e1fabbe05201300e'
out/part26h/run-preset-gate.cmd windows-msvc-direct2d $hTag
out/part26h/run-preset-gate.cmd windows-msvc-telegram-debug $hTag
out/part26h/run-preset-gate.cmd windows-msvc-win32-preview $hTag
out/part26h/provenance-gate.cmd $hTag
pwsh -NoProfile -File out/part26h/trace-includes.ps1 -EvidenceTag $hTag
pwsh -NoProfile -File out/part26h/verify-boundaries.ps1 -EvidenceTag $hTag
```

The wrappers run `cmake --preset`, `cmake --build --preset --parallel 4`, full
`ctest --preset --output-on-failure --no-tests=error --output-junit`, then verify
registration/result counts. They require EvidenceTag==HEAD: after a docs seal,
use that current full HEAD for a new run, not the old tag with a different HEAD.
Provenance uses `python scripts/verify_vendor.py --variant all` and
`python scripts/generate_tgs_compatibility_corpus.py --check`.

Root manually reviewed192generated library/executable link nodes (37none,79TG,
76preview), exact shared rule/flag bindings, Rendering/Runtime object ownership,
system-library operands, manifests/hooks and generated root/nested install
reachability. The none own stream executable does not link Reference or rlottie.
Private headers stay under src/, public export dependencies remain first-party,
and reference-linked installation remains prohibited. Actual include inventories
contain no vendor/reference/rapidjson header. Original graph/object hashes stayed
unchanged during syntax-only traces. Details: `out/part26h/root-source-boundary-review.md`.

Final pixel evidence is in preview `own-native-ellipse-capture-artifacts/run-5102284146700-0`:
180PASS/exact, minimum active IoU0.923077, maximum active MAE2.603750, maximum alpha
relative error0.014509. Actual counters: animated A/B create1/2 slots, A-repeat
hits1 with no creation; activity inactive creates none and retained visible replay
hits2; static B hits5 without a new slot. Preserved metrics are not a speedup claim.

Host remains clean at712d454a7c5c175ad59a3ca547c6b22ce8392da1, UI/out absent,
Release EXE SHA256 C92F26EE8FEB4FF03F6DC7F4AFFF4B11FB48142743D1EDCCB4E213AAA81F82C2
unchanged. No host writes/builds/GUI, automation changes or new external corpus.
These are incremental builds, not clean rebuilds. No hardware GPU, zero-allocation,
performance or fully-green Samsung claim is made.

## Evidence and process limitations

- Initial plain-shell baseline failed compiler discovery in the nested legacy
  subproject (36/37); the same unmodified suite under VsDevCmd passed37/37. Both
  attempts are retained. This is not product RED.
- Task1 initial compiling functional RED precedes implementation. The counter
  subcase was first reached by a later mutation RED; it is not separate strict
  test-first evidence. One failed `py` scratch launcher log was overwritten by
  the worker; the gap is disclosed, not reconstructed as original evidence.
- Task2's first compile error is retained but not counted as functional RED.
  The later compiling false-comparator stub produced the expected RED. An
  incorrect test expectation about invalid asset-handle packing was diagnosed
  and corrected to the actual planner contract; no product behavior or semantic
  comparison exclusion changed.
- Existing vendor rlottie C4251 warnings in focused and final reference test rebuilds are retained,
  not suppressed by an unauthorized dependency change.

Raw task output: `out/part26h/task-1`, `task-2`, `task-3`. Spec, detailed plan,
durable decisions/costs and ignored SDD task/review reports are referenced by
`docs/superpowers/STATE.md`. No evidence or historical worktree cleanup is done.

## Following stage

Next, separately design own scheduling/host adaptation for
the admitted subset, explicit producer identity and cache retirement; only then
perform an isolated static Avelabs acceptance. More primitives/general loading
and a source/rights-tracked external corpus need their own admission and parity
work. Avelabs release, UI/out and automation state are outside this stage's writes.
