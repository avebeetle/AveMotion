# Supported own UI implementation plan

> Required execution: subagent-driven development, one product writer at a time.

**Spec:** `docs/superpowers/specs/2026-10-11-supported-own-ui-design.md`

**Goal:** Preserve the accepted measurement and make the existing own engine
usable through public headers, demonstrated by one actual Avelabs navigation icon.

**Architecture:** Add opaque asset/playback in existing Rendering, fixed named
Direct2D own draw route, migrate H off private headers, share the current execution
thread among independent existing sessions, attach a thin own-authored TTS icon.

## Global constraints

Reuse existing Player, evaluator, planner and backend. No new scheduler, large
cache, format expansion or GPU redesign. Unsupported input remains explicit error.
No private engine headers in H consumers. Keep async preparation, transparency,
bounded work, safe lifetime and independent session state. Docking, navigation,
accepted physics and generic backend/public fallback policy are unchanged.
No dependency installs, Windows/settings changes, new licensing/publication,
foreign artwork commits or history rewrite. Main in place and ordinary scoped
pushes authorized. Preserve raw evidence and original binaries. Tests/goldens
must not be weakened. Update STATE/ledger after each reviewed block.

### Task 0: Freeze the accepted measurement

H only. Before product edits, mechanically preserve exact existing files from
`build/checks/baseline-2026-10-11-001`: probe/CMakeLists.txt, probe/measure.cpp,
collect.py, analyse.py, protocol.json under `tools/diagnostics/motion-baseline/`.
Use exact unchanged content, record hashes. Keep existing observer, no framework.
Add a concise README with configure/build/collection/analysis commands, toolchain,
source revisions, asset hashes and limitations. Derive commands from existing
successful command receipts/CMakeCache and CLI, not guessed flags.

Copy original probe EXE/PDB and installed EXE/PDB into a new exclusive ignored
`build/checks/.../frozen-original/` directory, preserving originals. Receipt must
include source/build revisions, SHA256, paths and exact kit hash identity. Verify
copies match originals and do not overwrite old evidence or publish binaries.
Run existing diagnostics unit tests plus compile/check the preserved Python CLI
and validate protocol JSON. No production source/build changes. Report full
commands/results/hashes in this plan's task-0-report. Root performs independent
task review before commit/push and starts Task 1 only after freeze is verified.

### Task 1: Public own playback and fixed backend boundary

E only. Add `include/avemotion/render/OwnPlayback.hpp` and matching implementation
in existing Rendering. Use opaque OwnAsset/OwnPlayback ownership; expose bounded
byte preparation with structured errors/metadata, current target fitting,
controls/snapshot/evaluateAt and existing Player add/lookup. Do not install private
headers. Preserve existing implementation behavior and null Runtime identities.
Add a named fixed-own-profile Direct2D draw entry without changing generic draw.

TDD: first real failing public-path supported-load/control/evaluation test,
including functional unavailable stub only if needed to make declarations compile.
Then minimal delegation. Cover unsupported/malformed/limits, immutable ownership,
two independent states, retention/removal/stale handles, failed evaluation, target
fitting and generic-vs-own preflight policy. Extend installed no-reference
consumer and required-header audit to prove actual own usage, retaining legacy
Runtime ReferenceUnavailable test. No Qt in core. Run full applicable MSVC/no-ref
CTest and installed package gate; report unexpected/pre-existing failures by name.
Root independent task review; scoped commit and ordinary push after fresh checks.

### Task 2: Consume only supported engine headers

H only, consumes Task 1. Migrate OwnMotionRenderer, MotionWorker, own smoke and
renderer tests to new public types/functions. Remove engine-private include
directories from `cmake/AveMotionLab.cmake`. Preserve current preparation/error
limits, fitting, WARP/target/caches/readback publication and 1/4/16 lab behavior.

Functional RED: a public-only compile/run consumer (no src include directories)
that prepares/renders a supported transparent animation; unchanged private route
must fail the boundary before migration. Verify explicit unsupported error and
already supported Duck think/X3/Party full available renderer/worker/page tests.
Build boundary evidence must demonstrate no private include paths/reference closure.
No changes to shared threading or actual icon yet. Root reviews before commit/push.

### Task 3: Shared executor and one actual navigation icon

H only, consumes Task 2. Add a small shared executor/service around the existing
MotionController/MotionWorker, keeping one Player/timer/mailboxes/renderer per
independent session. Queued initialization, per-session cancellation and worker
retirement, only final executor shutdown joins thread. No scheduling policy change.
Adapt existing lab controller to use it. Thin Qt animated-icon adapter reuses a
session and delivers owned premultiplied QImages to a QPushButton on GUI thread.
One own-authored admitted JSON resource adds an icon to the currently text-only
TTS navigation button, animates on hover/press, settles on its initial image at
rest, suspends when hidden, accounts for DPR; keep text,
checked state, navigation and existing button/window physics intact.

TDD functional RED for independent sessions sharing one actual thread, one survives
other destruction/replacement/error/hiding, no stale callbacks, no idle/hidden work.
Independence is state/lifetime isolation; serialized bounded work can delay another
session. Test unchanged state and eventual progress, not a fairness/latency guarantee.
Test real button interaction/transparent compositing/DPR/one navigation signal,
async readiness without paint waiting, image retention and asset sharing where
appropriate. Preserve existing lab worker/page/control tests and 1/4/16 coexistence.
Run all applicable local H tests; no GitHub Actions. Root task review before push.

### Task 4: Compare, whole-stage review and Release handoff

Both repos. Run E windows-msvc-win32-preview/full no-ref/public package gates and
H own/static Release full tests plus supported original Duck/X3/Party checks.
Freeze candidate source/build revisions and hashes. Build the preserved measurement
hook against candidate with minimal mechanical adjustment only if interfaces
require it; keep original source and old binary untouched and document adjustment.
Compare serial paired A/B runs with original protocol/observer and exact inputs,
actual raster/DPR. No builds/tests/workers during measurement. Use raw receipts and
report cold/warm differences, delivered batch versus image count, CPU/memory,
Qt-paint/group timing, pause/hidden behavior. Observe real icon-active/rest/hidden
work using existing observer/diagnostics. Slow result is evidence, not permission
for new scheduler/cache/format/backend work.

Dispatch broad independent final review after task gates. Fix/re-review findings,
then guarded installation in existing build/Release only with no running target
and recoverable validated backup; retain notices/symbols/import/source identity
checks from accepted delivery workflow. Automatic Qt interaction is not native
display proof; report native limitations and request only genuinely needed user
acceptance. Commit reports/STATE/checkpoints and ordinary push, final concise
summary with SHAs, commands, comparison and limits. Do not create/change schedules.
