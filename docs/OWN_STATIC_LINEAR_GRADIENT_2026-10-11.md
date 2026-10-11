# Own static linear gradients — 2026-10-11

## Outcome and checkpoint

The own model/scene and private Direct2D execution now support bounded static
RGB linear fills through the existing pipeline. This is a general capability
motivated by the unchanged Noto Party input, not a per-sticker renderer.
Both tasks are independently Approved with zero Critical/Important findings.
The strongest-model whole-stage review is also Approved/Ready to merge, with
no new actionable finding. Implementation, fresh gates, reviews and scoped
product pushes are complete. The plan is closed without a production fix wave.

Canonical source: `C:/Users/USER/Desktop/AveMotion`, directly on main under the
user's explicit delegation. Stage base: `4acb7bb28c6cd5d3fc3e622d005cd41b5be007e2`.
Design/plan: `6e0bf6130e0a79eb4331ae088e7794a760fdbbaf`; model/scene product:
`4e419ddc981796c1cbb17cf1f58c62195b727d79`; Task1 checkpoint:
`d7f7ed47ee99f40eb169062a1b8d51405b5ad30d`; private backend product:
`31d7665581d2f3dd29e592f6833fd6e36a931319`. All are ordinarily pushed.
The reviewed documentation checkpoint is
`cd493806cd579d771a26461b5ac48edc7c1ab80d`; subsequent closure documentation
does not change the tested product bytes.

## Scope and architecture

Group-local static linear RGB fills accept 2..16 strictly ordered stops, exact
`4*p` packed floats, and at most 4096 gradient floats per asset. Existing budgets
and shape scope remain unchanged. Keyed paint properties, alpha tails, radial
fills, gradient strokes, outer gradient paints, masks and mattes remain unsupported.

Local endpoints and stops retain static canonical identity. The full affine
matrix and separated inherited opacity flow through the existing own scene and
shared Direct2D executor. No second renderer, scheduler, registry, format or
dependency was introduced. Gradient resource creation/reuse/failure and context,
factory, domain, generation, clear/invalidation lifetimes are tested.

The build-tree-only `LinearGradientBackendAccess` fixes the private support
profile. It validates the entire requested plan and actual selected payloads
before native work. Unsupported or invalid items cannot draw a valid prefix.
Ordinary public draw still skips gradients; public planner unsupported accounting
and fallback policy are unchanged. This route is not wired into the installed UI.

## Tests-first and fresh verification

Task1 compiled functional RED rejected `gf` before production edits. Task2
compiled functional RED drew zero/skipped one, versus the required one/zero.
A test-only baseline shim used public draw before the private entry existed,
then switched unconditionally to the private entry. Compile/fixture-authoring
failures are retained separately, not counted as functional RED.

Task1 final frozen gates: no-reference 47/47, Telegram capture 95/95, explicit
Windows preview 96/96; both original Ducks' model/full-scene/capture checks pass.
Task2 final frozen gates:

| Existing MSVC Debug cache | Full CTest result | Test duration |
| --- | --- | --- |
| windows-msvc-direct2d, reference none | 47/47 | 10.35s |
| windows-msvc-direct2d-capture, Telegram | 95/95 | 247.80s |
| windows-msvc-win32-preview, Telegram/capture/preview | 96/96 | 274.54s |

All final build/test exits are zero, with no new skips. Preview selftest records
151 rendered frames, 48 unique frames, one resize and one device recreation,
final DPI144/800x450, WARP enabled. This exercises the existing preview lifecycle,
not installed gradient playback, a hardware-GPU path or performance acceptance.

Existing VS2022 x64 caches use MSVC19.44.35229.0 and CMake3.26.4. After entering
the existing VS developer shell, each gate uses:

```powershell
cmake --build out/build/<cache> --config Debug --parallel 4
ctest --test-dir out/build/<cache> -C Debug --output-on-failure --no-tests=error -j 1
```

Original Duck checks use existing Telegram-cache executables: own-vector model
`--tgs <input>`, stream differential `--tgs <input> --full`, and capture
`--tgs <input>`. All six commands exit zero. Each original retains 2960 visible
scene samples and 68 public-route captures, zero differing WARP pixels and no
skips, passing the unchanged CPU policy. Inputs were not converted or stripped.

## Independent raster evidence and qualifications

Ten owned fixtures cover 2/3/16 stops, fractional opacity, ordering/even-odd fill,
animated ancestry/geometry, reverse/repeat sampling, four rectangular/square
viewports, rotation and nonuniform scale. Final focused capture has 280 rows:
all native differences zero and all independent Telegram CPU comparisons pass.
Minimum IoU1, maximum alpha-relative error0.0126582, maximum whole-image mean
error0.0250549. Each full reference gate repeats 280 gradient rows successfully.
Thirty additional native host/presentation cases have no transformed CPU oracle.

The analytic diagonal witness inverse-maps pixel center `(20.5,2.5)` through
scale `(2,0.5)` to `(10.25,5)`, projecting on local `(0,0)->(20,20)` at
`t=.38125`. Measured RGBA is `(97,97,97,255)`; transformed-endpoint projection
would instead use `t=.4970588235`. The encoded-space midpoint measures
`(127,127,127,255)`, within the declared channel allowance3 and distinct from
linear-light interpolation near188. Direct2D uses GAMMA2_2 and clamp.

Independent native expectations author literal cubic commands, matrices, colors
and opacity. They do not read own sampled data or call backend helpers. Exact
equality is matched-command/float-recipe evidence, not a promise for every
equivalent tessellation. Earlier expected-line/color-recipe failures remain in
raw evidence. No production fix, golden update or tolerance relaxation hid them.

Ordinary Telegram gradient scenes expose final endpoints/stops but not the
local/separated paint seam. Final-field differential assertions and own literal
local/static assertions are therefore distinct, not full-field local parity.
Original Ducks retain previously qualified raw-local residuals: a-2 maximum
33.6390705109 with239805 over-limit fields; a-3 maximum0.0001220703125 with23115.
Compared final/predicted-local fields and unchanged raster policies pass.

## Provenance, retained evidence and next gate

No vendors, versions, notices, licenses, dependencies or Windows settings changed.
Original Party hash remains
`EAB1FD7070BFFD2965C12A80D2D91CFF76683AF65019F142ACF9A553EE877B5C`.
Both Duck hashes are retained in task reports and input-hash artifacts.
Artwork stays in the local corpus, not newly published or packaged.

Raw evidence: `out/own-linear-gradient-2026-10-11/task-1/` and `task-2/`.
Each task's `.superpowers/sdd/2026-10-11-own-linear-gradient/` brief, report,
frozen review package and ledger remain locally recoverable. Actual commands,
exits, cache/compiler identities, frozen hashes, CPU/native bytes/images and
failed iterations are preserved. Inherited vendor C4251 and Git line-ending
warnings are disclosed without suppression or unrelated vendor edits.

Avelabs remains clean at `e02c2eb4f641dd55fdf64a015eb0047277cc0dae`.
Installed `build/Release/AvelabsUI.exe` remains 52773888 bytes, SHA256
`FC3AEC2BB876BCF4C2F62F441BBD366C5CB728D2F75F14B3684990E52E6D6E4E`.
No new installed EXE, UI/manual acceptance, Release1/4/16 timing, speedup or
hardware-GPU claim belongs to this stage. Next capability: separately designed
alpha mattes for original Party, then unchanged-input acceptance, deliberate
host wiring and measured Release gates. Party is still unsupported now.

## Controller decisions and their costs

- Keep the static RGB profile and full local affine semantics. Other legitimate
  gradient variants and final-only plans without a coherent local pair fail.
- Share the existing executor through a fixed private entry, preserving public
  policy and cache architecture. Deliberate host wiring and measured optimization
  remain separate; this block is not installed Party playback or a speedup.
- Retain corpus/vendor/license/history bytes and all raw development evidence,
  rather than deleting the local SDD workspace. Cost: retained local artifacts,
  with no shipping or publication permission implied.
- Work directly in main as explicitly requested. Cost: a shared checkout rather
  than isolated worktrees, controlled by one writer and scoped commits/index.
- Qualify the ordinary reference's missing local/separated gradient seam.
  Cost: no full-field local-gradient parity promise; own literal assertions and
  ordinary exported-final assertions remain distinct.
- Validate all requested draws and actual selected payloads, conservatively
  rejecting scene-wide compositing. Cost: unused compositing also rejects this
  private route; unplanned paint is not whole-asset admission.
- Retain inherited warning noise and unchanged prior raw-local/Unicode/animated
  opacity concerns without unrelated cleanup. Cost: no warning-free or new
  coverage claim for those prior concerns.
- Match independent native authored cubic commands and float color recipe.
  Cost: exact native equality is narrower than equivalence across all recipes;
  independent CPU/literal checks remain mandatory, and extra transformed native
  cases do not establish transformed CPU parity.

Every final-review declined item is retained within these acceptance boundaries,
not silently dropped. No new authority, vendor edit or expanded feature was
needed to close the gradient stage.
