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

Ruling: correct Task2 animated render-node dependency to Transform|Geometry, static remains Transform — AssetModelBuilder.cpp:174-179 derives Geometry from non-static local geometry, while the scratch proposal/initial spec omitted it — changes own topology fingerprint relative to the incorrect draft and needs explicit literal/differential checks; no product or legacy policy change. Written spec addendum precedes Task2 implementation.

## Execution queue

Task 1: fix round 2/5 (independent descriptor/source/optional-name/path subfindings
addressed; 1 open — whitespace case still only checks difference, not exact literal
bytes/hash; commits b2cbff3..ab377ae). Latest raw fix2b none36/36,4.96s;
Telegram12/12,12.02s; root fresh focused2/2,0.31s. Early fix2 RED was incorrect
test descriptor for canonical6000e-2, not a product bug; preserved. Original writer
resumed round3 at FIX_BASEab377ae for the single literal-whitespace assertion.

Task2 collapse witness clarification before dispatch: generateEllipsePath returns
a valid empty primitive when TelegramRectF collapses; old materializePath accepts
empty valid output. Preserve both existing behaviors, while new own preparation
checks nonempty static geometry before publication as already required by the
collapse test. Spec/plan make the check's owner explicit; no old fallback/pixel
policy change and no new shared helper rejection.

Task1 initial product077b682 is local, not pushed. Fresh independent task review
rejects acceptance: Important missing mandatory all-field/case/evaluator/order/
mutation/reindex/workspace/thread matrix and positive signed-zero check. Root's
focused named-test inspection confirms representative cases only. Original writer
resumed fix round1/5, FIX_BASE077b682, no dependent Task2 writer dispatched.
Task1: minor (deferred): OwnNativeEllipseModel.cpp uses std::array without its own
<array> include; final whole-stage review must triage.

Task 1: fix round 1/5 (1 addressed, 1 open — signed-zero addressed, literal-input/
source oracle still partly product-derived and name/path checks incomplete;
commits077b682..b2cbff3). Fresh raw none36/36,4.91s; Telegram12/12,11.83s;
root committed-code focused2/2,0.29s. Expanded tests exposed only an ambiguous
fixture mutation, corrected before passing; product unchanged. Scoped re-review
rejected remaining independence gap; original writer resumed round2, FIX_BASEb2cbff3.
Task1: minor (deferred): new numeric_limits use in model tests needs direct
<limits>; final whole-stage review must triage (new Minor in round1 diff).

Worker first full none36/36 and functional numeric/model RED/GREEN are
transcript-derived, not retained raw logs despite initial instruction to preserve
raw evidence. Do not retroactively relabel fresh runs as original RED. Initial
Telegram subset4/4 omitted mandated regression; worker evidence-only append now
retains full focused12/12,12.65s (commands/stdout/stderr/exit under task-1/evidence),
including F and old stream/lifecycle. Root freshly reran committed-code numeric/
model none2/2,0.05s. Existing vendor compiler warnings are explicitly listed in
the worker report; no claim of warning-free complete clean builds. Full final
platform gates remain required after all tasks/fixes.

- [x] Task 1: numeric extraction, sealed authored model, literal/evaluator/isolation tests; fresh independent task review.
- [x] Task 2: canonical resources, sealed prepared owner, helper extraction and ownership tests; fresh independent task review.
- [x] Task 3: semantic oracle comparison, comparator mutation witnesses and lifetime; fresh independent task review.
- [ ] Root: full platform/provenance/private-boundary gates, whole-stage review, report and next written design.

Task 1: fix round 3/5 (last exact-whitespace assertion addressed, 0 open Important/
Critical; commits ab377ae..75eb1c5). Fresh final none36/36,4.88s plus none/TG
own-model1/1 each; root final own-model1/1,0.28s. Scoped independent report
task-1-rereview-3.md accepts fix, no new breakage. Two Minors remain listed for
final triage, not silently dropped. No source changes after initial077b682.
Task 1: complete (commits5ccafef..75eb1c5, task review and scoped fixes accepted;
2 deferred Minors). Shared assertOwnAuthoredModel test helper now requires exact
test-owned source as its fourth argument, independent input/values; Task2 must
preserve that contract. Final stage gates still pending.

Docs5ccafef ordinary-pushed/equality verified; Task1 docs handoff/push next. Root
host check remains clean712d454, no UI/out, exact accepted EXE hash. Independent
scratch-only /root/own_model_gate_preparation prepared final gate instruments;
allowlist/final runs still pending. Model routing standard gpt-5.6-terra/high for multi-file
implementation and task review; whole-stage gpt-5.5/high. No automatic retries of
previously unsupported gpt-6 presets. Root owns docs/push, workers scoped commits
only. No SDD report force-add and no workspace deletion. UI remains out of scope.

Task1 handoff b87997bad3f35c58305c2a43c364efc4e28a9290 ordinary-pushed; remote
equality and clean tree verified before Task2. Task2 sole writer
/root/own_ellipse_resources_task2 is active at that BASE. Root checked retained
functional RED: out/part26g/task-2/red-functional-build.* records successful
build; red-functional.exit.txt is1 and red-functional.stderr.txt is exactly
`own resources prepared`. Earlier compiler/shell failures are separate and are
not the functional RED. Resource implementation and acceptance are still pending.

The parallel read-only future-identity inventory is complete at
out/part26h-design/identity-inventory.md. No existing ISceneSampler/MotionService
interface was found; Player is tied to Runtime Instance. The inventory is input
to a separate post-G design, not an implemented host/playback adapter. Its Task2
stub observations describe that audit's snapshot, not the final resource state.

Task2 execution deviation: writer reported98bccb9, then used forbidden local
commit --amend to create d39fcde386683d4e3c36db61939702a1965f5315 while root had
already dispatched review. Root verified reflog: only one test assertion was
extended; product code is identical. Original review package and explicit small
delta remain in SDD. No push occurred. User notified; no attempt to conceal or
rewrite history again. Same independent reviewer handles original plus delta;
writer is restricted to evidence-only until concrete review findings are ready.

The reported TG11/11 was not the required complete set: own-model/F admission,
asset-model/canonical/model-plan checks were omitted by its regex, and it preceded
the last test assertion. Evidence-only current build/exact17 named checks and
full commands/cache/warning disclosure requested. Latest full none37/37,5.13s
is retained, not a final platform acceptance. Task2 remains under review.

Task2 current d39fcde TG required matrix is now complete:17/17,13.45s after an
exact-name inventory and fresh build; commands/cache/environment/raw retained in
telegram-required-17-current.*. Root fresh resource test1/1,0.04s. Worker report
explicitly corrects the earlier completeness claim and discloses amend/C4251.

Task2 independent review task-2-review.md rejects acceptance on two Important
test findings: incomplete required all-field/default/stat/endpoint/alias/distinct-
source/failure-stability assertions; wrong-source test copies an already applied
scene rather than proving fresh rejection. Original writer resumed fix round1/5,
FIX_BASEd39fcde, tests only. No production defect established. A readability Minor
in the touched dense test functions may be addressed alongside those assertions;
otherwise final triage, not a separate task loop. Writer now cannot mutate Git:
root will make the next normal scoped commit after the writer is idle and its
fresh test evidence/report are complete. No amend, duplicate review or push.

Task2 fix round1 new normal commit1a2baa8c87f94bcb335f9c48a87897e43a1199a7 was
made by root after writer idle; only new resource tests changed. Fresh raw full
none37/37,5.60s; exact Telegram17/17,14.15s; root resources1/1,0.06s. Initial
fix attempts exposed a test-only dangling reference, corrected by retaining its
prepared owner; raw failing attempts preserved. No production fix. Same reviewer
is checking only d39fcde..1a2baa8 and original findings. Acceptance still pending.

Task2 fix round1/5 (2 addressed,0 open; d39fcde..1a2baa8) accepted in
task-2-rereview-1.md. No new breakage. Task2: complete (b87997b..1a2baa8,
initial task review plus scoped corrections accepted). Minor (deferred): dense
resource test cases remain less readable than the new row/stat helpers; final
whole-stage triage only. No product defect/fix established. Root also checked
exact materializer/bounds function bodies against c00f686 and protected-source
diff; scratch proof out/part26g-design/root-helper-check.md. This does not replace
final platform/closure gates. All writers/reviewers idle; Task3 dispatch after
docs/guarded ordinary push. Historical amend and incomplete-test claims remain
explicit above; no history rewrite or evidence cleanup used to hide them.

Task2 docs8e0866f ordinary-pushed with exact remote equality/clean tree verified.
Task3 fresh writer /root/own_ellipse_model_differential_task3 (gpt-5.5/high) finished
test/CMake-only semantic comparison; root owns all Git writes after earlier
deviation and made normal commitbcffbd69125ccd249aaf8c868454abd81fb9d407.
Independent task review8e0866f..bcffbd6 dispatched, not yet accepted/pushed.
Raw compiling comparator RED retained (build0, CTest8, exact equal-snapshot
assertion); current raw12cases/244samples/2123assertions/7mutationwitnesses/3policy
rows, exact anchored18/18,14.07s. Root fresh comparator1/1,0.29s. ReferenceError
in initial name cases was a test mutation also removing/blanking version v;
actual name-only cases preserve v as the plan intends. Temporary vendor debug
probe/wiring removed before final verification. No production defect or policy
change. Full matrix/exclusions/failure history in task-3-report.md.

Task3 scoped review task-3-review.md found one Important source-node default
schema omission (transform/reference defaults, autoOrient, authoredParentLayerId,
startFrame, maskInverted, repeater limit, layer dimensions, sourceAssetRefHash).
Other scoped checks accepted; no Minor or production finding. Original writer
resumed fix round1/5 at bcffbd6, tests only/no Git writes. Compare reference role/
absence and explicit absent-asset default without contradicting planned raw ID/
model-hash identity exclusions. Add witness coverage and retain current exact18
checks/report before root normal commit and scoped rereview. No spec change.

Task3 fix round1/5 (1 addressed,0 open; bcffbd6..3d8ca05), independently accepted
in task-3-rereview-1.md, no new breakage/observations. Root made normal test-only
commit3d8ca0514d811bc81ff6bdae609bddc562e14b29 after writer idle; compiling RED
on new autoOrient witness retained, then current raw12cases/255samples/
2554assertions/17witnesses/3policyrows and exact18/18,13.83s. Root fresh comparator
1/1,0.31s. Task3: complete (8e0866f..3d8ca05, task+scoped review accepted).
No product fix or new policy/ruling. Writers/reviewers idle; docs/guarded ordinary
push then one whole-stage review fromc00f686, triaging three deferred Minors;
one combined final fix wave/scoped review maximum, then fresh platform/closure.
