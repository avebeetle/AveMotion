# Part26H — own ellipse scene stream

Status: in progress. Tasks1/2/3 accepted; final independent/platform gates remain
pending. This document is not a completion claim.

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

## Task evidence accepted so far

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

## Final gates

Pending frozen product HEAD, fresh full none/Telegram/Win32-preview gates,
Samsung configure/listing plus provenance, actual new-source MSVC include traces,
generated graph/install review and unchanged-host recheck. No speedup, hardware
GPU, zero-allocation, clean-rebuild or fully-green Samsung claim is made.

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
- Existing vendor rlottie C4251 warnings in focused test rebuilds are retained,
  not suppressed by an unauthorized dependency change.

Raw task output: `out/part26h/task-1`, `task-2`, `task-3`. Spec, detailed plan,
durable decisions/costs and ignored SDD task/review reports are referenced by
`docs/superpowers/STATE.md`. No evidence or historical worktree cleanup is done.

## Following stage

After H passes all gates, separately design own scheduling/host adaptation for
the admitted subset, explicit producer identity and cache retirement; only then
perform an isolated static Avelabs acceptance. More primitives/general loading
and a source/rights-tracked external corpus need their own admission and parity
work. Avelabs release, UI/out and automation state are outside this stage's writes.
