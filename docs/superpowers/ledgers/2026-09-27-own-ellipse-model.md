# Part26G own ellipse model — durable execution ledger

Plan: docs/superpowers/plans/2026-09-27-own-ellipse-model.md
Spec: docs/superpowers/specs/2026-09-27-own-ellipse-model-design.md
SDD: .superpowers/sdd/2026-09-27-own-ellipse-model/progress.md
Stage base: c00f686450c1fd725b39e543badd42cdf23df20c (accepted Part26F).
Spec commit: efc12e9e04f42f4246c985cbcd7a205cf84b6d95.

## Preflight and delegated approval — 2026-09-27

Root completed source/design review and plan self-review. Spec is binding;
controller approves plan/SDD under explicit user delegation, not a claim that the
user reviewed these unseen documents. Git main, git-dir==common-dir; no live writer
or build at preflight. Remote is the accepted c00f686. No previous G tasks exist.
Plan fixture shorthand corrected to actual telegram_sticker_basic.tgs before dispatch.

| Task/pair | Produced versus consumed / internal check | Result |
| --- | --- | --- |
| 1 | Optional numeric values retain binder's conversion order; sealed document-only own graph, no render dependency; numeric/model compiling RED precedes production | Compatible with F input/owned source and exact existing binder; no admission weakening |
| 2 | Sealed authored input, copied full graph, private Model helpers and identical path materializer; literal resource/ownership tests versus construction | Rendering consumes Runtime, not reverse; null/collapse failures publish nothing; no certificate substitution |
| 3 | Semantic DTO and comparator witnesses versus two independently built graphs; deliberate policy rows outside common matrix | No raw-ID/fingerprint equality requirement or mismatch whitelist; no pixel/full-playback assertion |
| 1 → 2 | OwnNativeEllipseModel and values/binding/model fields; CMake registrations and new test helper shared | Exact const interface in both tasks; sequential writer preserves Task1 tests; resource refresh preserves authored fingerprint |
| 1 → 3 | Numeric/model factory and semantic binding/evaluator workspaces; CMake/helper | Explicit per-model workspace and source ownership; role-resolved comparison, literal tests remain independent |
| 2 → 3 | Prepared asset/model, canonical resources and Model application; CMake/helper | Own prepared object not Runtime Asset; oracle object kept distinct; matching semantics only |
| 1/2/3 → root | Existing/new tests, private sources and preserved old routes | Root final platform/closure gates plus broad review after sequential task reviews; Samsung configure-only |

No mandates to duplicate production algorithms, change existing tests/goldens,
weaken certificate/loader policy, or alter UI/Windows/automation were found.
The intentional test comparator stub is a functional RED step, not final behavior.

## Decisions and costs

Ruling: split own authored construction in Runtime from resource preparation in Rendering — reuse existing targets without a dependency cycle — costs a second private interface and integration tests if future ownership requirements differ.

Ruling: extract one compiled numeric interpreter from the existing binder unchanged; representability is separate from exact admission — avoids divergent float policy without weakening F grammar — accepted tiny values can still yield a typed preparation rejection, requiring callers to distinguish these outcomes.

Ruling: use sealed own owners with invalid Runtime handles, never fabricated Assets or weakened reference certificates — provenance of direct construction is distinct from scan certification — own stream integration needs a separately designed identity boundary next.

Ruling: own source-derived versioned parsed fingerprint and literal render labels need not equal Telegram row/fingerprint/key-path conventions — current evaluator ownership and display labels do not require oracle identity — whitespace changes workspaces' fingerprint and future selector APIs need their own escaping design; fingerprints are not security tokens.

Ruling: retain authored owner plus a small immutable full-model copy — simple, bounded publication and lifetime proof — duplicates authored graph storage; no memory improvement claim.

Ruling: expose private wrappers around existing Model hashes/statistics and mechanically extract the stream path materializer — reuse first-party math without copying algorithms or changing emission authorization — unchanged legacy behavior needs regression and helper diff verification.

Ruling: work directly on main and retain ignored raw/SDD artifacts under explicit user instructions — preserves user's chosen workflow and recovery evidence — less isolation and retained disk use; scoped staging/guarded pushes remain mandatory.

## Execution queue

- [ ] Task 1: numeric extraction, sealed authored model, literal/evaluator/isolation tests; fresh independent task review.
- [ ] Task 2: canonical resources, sealed prepared owner, helper extraction and ownership tests; fresh independent task review.
- [ ] Task 3: semantic oracle comparison, comparator mutation witnesses and lifetime; fresh independent task review.
- [ ] Root: full platform/provenance/private-boundary gates, whole-stage review, report and next written design.

Task 1: pending dispatch. Model routing standard gpt-5.6-terra/high for multi-file
implementation and task review; whole-stage gpt-5.5/high. No automatic retries of
previously unsupported gpt-6 presets. Root owns docs/push, workers scoped commits
only. No SDD report force-add and no workspace deletion. UI remains out of scope.
