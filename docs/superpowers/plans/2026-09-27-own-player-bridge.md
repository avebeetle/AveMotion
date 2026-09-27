# Part26J Own Player Bridge Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Register own ellipse playback with the existing Player without a second scheduler.

**Architecture:** Private owning source/table inside Player; Rendering-private typed add/lookup functions. Keep public Player verbs and scheduling implementation.

**Tech Stack:** Existing C++20/MSVC2022/CMake/Ninja/CTest; no new dependency.

**Spec:** docs/superpowers/specs/2026-09-27-own-player-bridge-design.md (5489760).

## Global Constraints

- Main/root-only Git, no force/rebase/history rewrite, preserve raw/SDD.
- No host writes/build/GUI or UI/out recreation in J; no vendor/licenses/dependencies, goldens/fixtures, public own loader, Player threading, fallback or D2D ownership changes.
- Existing public scheduling behavior and Samsung policy stay intact.
- One product writer; workers never stage/commit/push or dispatch agents. Root does Git/reviews/docs.
- No public producer API, fake Instance/handle, second registry/map/scheduler/timer/thread or per-entry adapter heap allocation.
- Mixed scheduling does not permit mixed own/Runtime planner/backend namespaces. Explicit cache retirement and linked-allocator/single-writer contracts remain.
- Raw unique attempts under out/part26j; reports/reviews in this plan's SDD workspace. Every configure/build/CTest runs after VsDevCmd in the same process.

## Review Focus

- Wrong type lookup in mixed registration: exact own-table identity before cast (Task2).
- Invalid source publishes work/changes counters: reject atomically before registration (Task1).
- Hidden user pause wrongly auto-resumes: Freeze and manual control interactions (both).
- Source removal leaves stale host wakeups or drops pending repaint during callback replacement (Task1).
- Once snapshot commits too early or failed evaluation loses source lifetime/cache retirement (Task2).

### Task 1: Internal owning source binding in existing Player

**Files:** create src/player/PlayerSource.hpp and tests/player_source_tests.cpp;
modify src/player/Player.cpp, include/avemotion/player/Player.hpp (opaque detail
friend only), CMakeLists.txt. Existing tests/player_tests.cpp only for missing
legacy characterization. Do not reorganize other Player functions.

**Interfaces:** namespace player::detail, PlayerSourceOps with eleven noexcept
function pointers taking void* (snapshot/rate const void*): frameRate()->double,
snapshot(MotionTime)->PlaybackSnapshot, play/pause/resume(MotionTime), stop(),
seekNormalized(double,MotionTime), setControlledProgress(double),
setDirection(PlaybackDirection,MotionTime), setPlaybackRate(double,MotionTime)->bool,
setLoopMode(PlaybackLoopMode). PlayerSource holds shared_ptr<void> owner,
const PlayerSourceOps* ops=nullptr, genuine runtime::Instance* runtimeInstance=nullptr,
runtime::InstanceHandle runtimeHandle defaultinvalid. Table static lifetime.
PlayerSourceAccess::add(Player&,PlayerSource,MotionTime,PlayerEntryOptions={})
->PlayerAddResult; ::source(const Player&,PlayerHandle)->PlayerSource (empty on
invalid). This is a private header, not installed/exported API. Public Player
only gains detail forward declaration and friendship. Own adapter in Task2 uses
these exact field names/signatures; source operations name snapshot internally.

- [x] Write no-ref behavioral tests around real Player using a test-only source
  backed by the existing PlaybackControl plus explicit rate1/duration1. Test
  table implements concrete controls, not a duplicate scheduler. Compiling stubs
  for private add/source must fail the assertion that a valid source can return
  its initial scheduled frame; capture functional RED before implementation.
- [x] Implement private table validation and registration. Public Runtime
  addInstance builds its static table and source, retaining old null/error
  messages/options/duplicate validation semantics. Route only source operations
  through the table; keep arithmetic, storage, registry, handle generation,
  lifecycle and callback policies unchanged. Runtime instance lookup aliases its
  retained genuine owner; generic source frames keep null/invalid Runtime fields.
- [x] Literal tests: first stopped tick has one FirstFrame/PlaybackChanged frame,
  next unchanged tick empty; play at0 with rate1/source1 gives deadline1s;
  tick3.5s yields one TimelineAdvanced frame, skipped2, next4s. Invalid source,
  missing callback, invalid maximum rate and duplicate owner mutate no registry
  or host callback state. Explicit invalidations coalesce. Invalid/stale handles
  do nothing; remove/clear release owner, cancel final wakeup but request repaint.
  Callback replacement preserves that pending repaint; destructor/move retires
  old callbacks. Hidden Freeze with manual pause does not resume; KeepUp avoids
  automatic pause. Exercise diagnostics/reset storage and current counts.
- [x] GREEN focused new test avemotion.player.source (unconditional), then fresh
  full none and Telegram suites; Samsung targeted avemotion.player.scheduler
  and avemotion.player.source. Keep known vendor warnings visible.
- [x] Self-review/report task-1-report.md with commands/raw RED/GREEN/actual
  results and concerns; root focused check, scoped commit, independent task
  spec+quality review, STATE/ledger update and guarded ordinary push.

### Task 2: Typed own registration and end-to-end scheduling tests

**Files:** create src/render/OwnNativeEllipsePlayer.hpp/.cpp,
tests/own_native_ellipse_player_tests.cpp,
tests/own_native_ellipse_player_differential_tests.cpp; CMakeLists.txt.
No new public methods or Task1 changes without root ruling.

**Interfaces:** render::detail::addOwnNativeEllipsePlayback(player::Player&,
shared_ptr<OwnNativeEllipsePlayback>,runtime::MotionTime,
player::PlayerEntryOptions={}) -> player::PlayerAddResult;
ownNativeEllipsePlayback(const player::Player&,player::PlayerHandle)
->shared_ptr<OwnNativeEllipsePlayback>. Consume Task1 PlayerSource fields/ops.
Static own table, no extra owner allocation/registry. Rendering PRIVATE links
Player; Player still depends only on Runtime. Private src/player include path.

- [x] Write own-only tests against compiling add/lookup stubs; valid prepared
  JSON->own playback registration must initially fail to yield a scheduled own
  frame, observe functional RED. Reuse existing fixtures and preparation helpers.
  Register avemotion.own_native_ellipse_player unconditionally; TG-only
  avemotion.own_native_ellipse_player_differential.
- [x] Implement thin typed add and lookup. Lookup checks exact own table pointer
  before casting; null/wrong-type/stale returns null. Document borrowed tick
  spans, invalid Runtime fields, scheduling vs scene/cache IDs, caller forget/
  reset, own-only graphics domain, serial calls and allocator lifetime. Do not
  implement evaluation, rasterization or Qt in this adapter.
- [x] No-ref real N61/fr60 own literal proof: first frame0; play/tick250ms ->15;
  pause freezes15; resume/seek/reverse/loop and rate changes use existing controls.
  Freeze hide250ms/show1250ms preserves.25, next250ms ->.5; user-paused stays
  paused; KeepUp continues logical time without hidden frames. Early tick emits
  none; late tick skips deadlines rather than issuing catch-up frames. Verify
  callbacks/counts/diagnostics, direct own mutations detected by revision, one
  source registered twice rejected, independent two-source controls/owners,
  stale handle/re-registration, all own frame Runtime fields invalid/null.
- [x] Once at2s: scheduled completion snapshot does not commit own revision;
  invalid viewport evaluateAt fails without commit; explicitly invalidate for
  host retry (no implicit new Player retry policy), valid evaluation commits
  exactly once. Retained scene/plan survive remove and owner destruction;
  planner forget rebuilds isolated own resource state. Player itself never
  claims cache eviction. Clear/replacement/destruction cancel wakeups and release
  registered owners. No new lifecycle hooks used only by tests.
- [x] Telegram mixed-player trace uses actual Runtime and own instances over
  existing admitted fixtures, same options/controls/time. Assert typed lookup
  rejects Runtime entry; Runtime instance() rejects own entry; legacy frame
  pointer/handle preserved, own fields empty, distinct Player handles, snapshots/
  reasons/deadlines/counts align where semantics align. This is shared-scheduler
  wiring proof; literal tests independently fix expectations. Own scene emission
  and planner used only in own domain; never compare IDs by casting.
- [x] GREEN focused then full none/TG, Samsung new own/source/old scheduler tests.
  Root focused check/commit, independent task review, update STATE/ledger and
  guarded ordinary push. Report exact outputs/warnings in task-2-report.md.

## Final gates and handoff (root)

- [ ] Whole-stage review084d760..HEAD on most capable model; deferred findings
  visible, at most one combined final fix and one scoped re-review. Root explicitly
  disposes every declined-to-judge item rather than treating it as approval.
- [ ] Freeze HEAD and run sequential MSVC configure/incremental-build/full CTest
  windows-msvc-direct2d, windows-msvc-telegram-debug, windows-msvc-win32-preview;
  Samsung targeted source/own/old scheduler plus playback/frame_mapping. Both
  vendors/TGS verification; retain successful full CTest logs as well as JUnit.
- [ ] Actual MSVC includes Player.cpp/new own adapter in none/TG, no-ref actual
  links and install/export closure: friend alone installed, private headers not;
  Rendering->Player no cycle. Preserve exact input/instrument hashes. Read-only
  host clean/current EXE checkpoint; no UI/out recreation or host build.
- [ ] Final report/STATE/ledger, docs-only identity check, guarded ordinary push.
  Describe completed scheduler bridge and next own scene/plan+offscreen Qt pilot,
  not completed visual integration/fullengine/performance. Do not revive schedule.

## Approval/self-review

Controller reviewed this plan against spec and approves SDD under delegated
authority. Two source consumers justify this one internal seam; no duplicate
scheduler/facade registry. All focus risks assigned to tests; no additional
public loader, rasterizer or UI stage hidden in J. Root owns all Git writes.
