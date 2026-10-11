# Own static linear gradient implementation plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development to implement this plan task-by-task. Steps use checkbox syntax for tracking.

**Goal:** Load, emit and privately render bounded own static linear RGB fills with full affine semantics, without changing public MVP rendering policy.

**Architecture:** Task1 extends the existing authored compiler and shared own scene program. Task2 adds a fixed private entry to the existing Direct2D executor and tests it against independent native and CPU evidence. No second renderer, scheduler or registry is introduced.

**Tech Stack:** Existing C++20, MSVC2022 x64, Direct2D/WARP, existing Telegram reference only in test builds.

**Spec:** `docs/superpowers/specs/2026-10-11-own-linear-gradient-design.md`.

## Global constraints

- Canonical source `C:/Users/USER/Desktop/AveMotion`, base4acb7bb, directly on main under delegated controller approval; no force/history rewrite.
- Static linear RGB, 2..16 strictly ordered stops, exact4*p floats, asset gradient-float cap4096; existing budgets unchanged.
- No opacity tails, animated paint properties, radial/gradient stroke, outer gradient paint, mask/matte, shape-scope or public fallback widening.
- Ordinary public backend draw and public planner MVP unsupported accounting remain unchanged.
- No installed Avelabs change or host delivery before separate original-input acceptance; no new download/licensing/publication/dependency/Windows/automation changes.
- Preserve raw failures and existing goldens/tolerances/vendors/notices; WARP is software, not a performance/hardware proof.

## Review focus

- Nonuniform ancestry with rotation must use local endpoints/full affine matrix, not a projection onto transformed endpoints; Task1 literal scene and Task2 interior-pixel witness.
- Fractional fill/group/layer opacity must preserve pinned rounding and transform-parent opacity0 must not hide a child; Task1 literal/differential witnesses.
- A malformed private plan after a valid solid must not draw the prefix; Task2 preflight witness with zero native draws.
- Domain/generation/factory/context changes must recreate gradient resources and restore the host transform on failure; Task2 fake lifetime/failure witnesses.
- Static paint resources must survive animated geometry, reverse seeks and presentation-only changes; Task1 keys and Task2 creation-count witnesses.

## Task 1 Own gradient model and scene

Completed and independently Approved,0Critical/Important. Product
4e419ddc981796c1cbb17cf1f58c62195b727d79 ordinarily pushed; actual full gates
47/47,95/95,96/96 and both original Duck regressions pass on frozen bytes.
Report/qualified oracle and retained warning evidence are in the current ledger.

**Files:**

- Modify `src/runtime/OwnVectorModel.cpp` for group-local gf admission, validation and gradient statistics.
- Modify `src/runtime/OwnVectorPropertyCompiler.hpp/.cpp` for bounded static packed gradient values.
- Modify `src/render/OwnSceneProgram.hpp/.cpp` for gradient property bindings/sampling.
- Modify `src/render/OwnNativeEllipseStream.cpp` for final endpoints/alpha and accurate scene counts.
- Create `tests/support/OwnVectorGradientTestData.hpp` with owned rectangles/paths, no corpus artwork.
- Modify `tests/own_vector_model_tests.cpp`, `tests/own_vector_stream_tests.cpp`, `tests/own_vector_stream_differential_tests.cpp` and named comparison/oracle helpers only if they need explicit gradient assertions.

**Interfaces:**

- Consumes existing `buildOwnVectorModel`, `prepareOwnVectorAsset`, `lowerOwnVectorProgram`, `sampleOwnScenePaint` and own stream APIs; no public model/evaluator changes.
- Produces `PropertyCompiler::staticGradient(SourceNodeId owner, Value property, std::size_t stopCount) -> PropertyId`, existing GradientStops semantics/tables, and internal gradient bindings on OwnSceneDraw.
- Task2 consumes static canonical local paint, coherent local geometry/paint space, full geometry transform and separated inherited opacity.

- [x] Write independent model/stream tests before production edits. Base owned fill has local start(2,3), end(18,3), stops0/0.5/1 and middle RGB(0.5,0.25,0.125), expected bytes(127,63,31). Assert GradientFill/Linear, static property flags, exact packed ranges/statistics, no FillColor, canonical local endpoints/stops and neutral inactive fields.
- [x] Add malformed/range/type/order/cap tests: p1/17/fractional, count mismatch/tail, keyed k despite a0, nonzero/false/type-invalid a, wrong scalar types, duplicate/descending/out-of-range offsets/channels, unknown gf/g/property fields, t2/0/3, gs/outer gf, hidden/blend/highlights, coincident endpoints, 4096-float boundary/overflow. Rejection has no asset and a contextual path/message; existing solids remain valid.
- [x] Add literal ancestry test: parent position(5,7), scale(200,50), opacity0; group translation(2,4), scale(150,150); child own opacity100. At identity viewport, expected final start(15,11.25), end(63,11.25), one visible gradient. Add fractional fill/group/layer opacity tables using explicit pinned float-operation expectations, animated parent/shape forward/reverse/repeat, precomp occurrence clocks and presentation/static paint identity checks.
- [x] Build and run model/stream tests against unchanged production. Retain successful build and functional rejection at gf as RED; compiler/test-authoring failures are not RED. Existing MSVC cache commands below, capture exact exits/output in a new task-date evidence directory.
- [x] Implement the bounded compiler/staticGradient writer, bind/sample gradient properties and final scene projection. Preserve full local canonical paint; final endpoint export uses the paint-scope affine and final alpha follows pinned composition. Do not alter scalar temporal tracks or parent-opacity rules. Enforce aggregate budget before append.
- [x] Add fresh ordinary-reference scene differential fixture cases with explicit gradient comparisons and mutations of endpoint, stop position/color/alpha and inherited opacity. Record any representation/raw-local qualification; do not weaken existing comparator/tolerances or use stripped Party as acceptance.
- [x] Run focused GREEN, then fresh full no-reference, Telegram and explicit preview suites on frozen product bytes. Existing capture cases remain unchanged in Task1. Recheck original Ducks with the existing external model/full-scene/capture commands, preserving hashes.
- [x] Self-review/report hashes, actual commands/results/iterations/limits; controller independent task review, STATE/ledger, scoped commit and ordinary push. Model/scene success is not private rendering or Party/UI acceptance.

## Task 2 Private Direct2D linear fill execution

**Files:**

- Create `backends/direct2d/LinearGradientBackendAccess.hpp` with fixed build-tree-only access class.
- Modify `backends/direct2d/Direct2DBackend.cpp` and private declarations in `include/avemotion/backends/direct2d/Direct2DBackend.hpp` only; public method/diagnostic layouts stay unchanged.
- Modify `tests/direct2d_contract_tests.cpp`, `tests/direct2d_contract/include/d2d1_1.h` and helper fake definitions for the actual common ID2D1Brush/gradient interfaces.
- Modify `tests/own_vector_capture_tests.cpp` and owned gradient test-data/support only for new cases and independent native pixel witnesses.
- CMake target-private include/test registration edits only if required by these exact files; no dependency/export/install policy change.

**Interfaces:**

- Consumes Task1 coherent local gradient/canonical paint and existing MotionRenderPlan/Backend/RenderSession.
- Produces `backends::direct2d::detail::LinearGradientBackendAccess::draw(Backend&, const render::MotionRenderPlan&, const RenderSession&) -> DrawResult`.
- Ordinary `Backend::draw` delegates to the shared executor with linear fill disabled; private access preflights its fixed support profile and enables it. No mutable global/per-backend public mode or claimed-own provenance gate.

- [ ] Write behavioral tests first: a valid linear fill must draw once; current executor draws0/skips1. Because the new entry does not exist yet, a test-only dispatch shim uses the current public executor for this baseline. Record this exact seam migration; after the private entry exists, the shim must call that entry unconditionally. This is not a production fallback or evidence that a missing API existed during RED.
- [ ] Run the compiled behavioral RED and retain output/exits. Independently retain ordinary public gradient skip test. Do not call compile errors functional RED or change expected draw/skip counts when switching the test dispatch to the new entry.
- [ ] Implement fixed private preflight plus shared executor. Reject unsupported/invalid content before any item draw, including valid-prefix plus radial/matte/dash/missing-local/corrupt-reference plans. Preserve ordinary public draw skip behavior and planner unsupported accounting; do not blanket-ignore unsupportedFeatureItemCount in a host.
- [ ] Implement cached clamp/GAMMA2_2 stop collection and linear brush by existing paint slot/hash/revision. Static stop/endpoints are local; use existing geometry/presentation/host transform once and inherited opacity outside static resources. Share geometry/stroke/host/domain ownership; gradient draws do not increment solidItemsDrawn.
- [ ] Extend contract fakes to actual base-brush signatures. Test repeated/presentation/opacity draws reuse resources, changed paint content replaces them, equal-content distinct identities don't corrupt resources, domain/generation/factory/context changes and clear/invalidate recreate, gradient resource creation failures return errors and restore saved transform. No new public diagnostic fields.
- [ ] Run focused GREEN. Add private WARP cases for 2/3/16 stops, fractional opacity, draw order/fill rules, animation/reverse/repeat, rectangular viewports and rotated/nonuniform ancestry. Require zero skipped admitted items. Existing cases retain public entry and unchanged evidence policy.
- [ ] Add independent literal/native rectangle: black-white diagonal local(0,0)→(20,20), ancestry scale(2,0.5); compare an interior pixel against inverse-local projection, not transformed-endpoint projection (quantization allowance <=3 channels). Also black-white horizontal midpoint distinguishes encoded-space interpolation from linear-light gamma. Use independent pinned CPU render with unchanged capture thresholds; same-backend equality alone is insufficient.
- [ ] If ordinary-reference plans lack coherent local space, preserve the failure/evidence; do not graft own fields or rewrite oracle provenance. An independently authored native expected matrix/brush may supply exact WARP evidence, but label its scope and retain CPU proof. Any unresolved affine/CPU parity blocks rendering completion.
- [ ] Freeze product/test bytes. Run all three full gates and original Ducks; retain real cache/build/source provenance and preview resize/device recreation. Controller task review, STATE/ledger, scoped commit and ordinary push.

## Final stage handoff

- [ ] Independent strongest-model whole-stage review of a single generated base..HEAD package plus reports, with named outside-diff checks only; no automatic full-suite replay by reviewers.
- [ ] Resolve all Critical/Important findings together with resumed implementer, fresh covering checks and one scoped re-review. Record every controller ruling/cost in ledger and final response.
- [ ] Verify exact frozen bytes, scoped clean Git, unchanged installed host and ordinary remote equality. Publish report/STATE with source commit identities, commands, actual counts, qualification of WARP/CPU/raw-local results and remaining matte/host/performance gates.

## Verification commands

Existing VS2022 x64 caches, no configure/dependency installation:

```powershell
& 'C:/Program Files (x86)/Microsoft Visual Studio/2022/BuildTools/Common7/Tools/Launch-VsDevShell.ps1' -Arch amd64 -HostArch amd64 -SkipAutomaticLocation
cmake --build out/build/windows-msvc-direct2d --config Debug --parallel 4
ctest --test-dir out/build/windows-msvc-direct2d -C Debug --output-on-failure --no-tests=error -j 1
cmake --build out/build/windows-msvc-direct2d-capture --config Debug --parallel 4
ctest --test-dir out/build/windows-msvc-direct2d-capture -C Debug --output-on-failure --no-tests=error -j 1
cmake --build out/build/windows-msvc-win32-preview --config Debug --parallel 4
ctest --test-dir out/build/windows-msvc-win32-preview -C Debug --output-on-failure --no-tests=error -j 1
```

Task1 focused CTest regex: `avemotion_own_vector_(model_tests|stream_tests|stream_differential_tests)`; inspect cache test listing for the exact registered names before invoking. Task2 focused backend contract plus own-vector capture. Original a-2/a-3: model `--tgs`, ordinary scene `--tgs --full`, capture `--tgs`, using existing Telegram-cache executables and unchanged input hashes. All required command exits must be0 with no new skips; report actual test totals, never assume historical totals are the current result.
