# Own alpha matte implementation plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Support the bounded own normal-alpha profile and verify unchanged Party before guarded Avelabs Release delivery.

**Architecture:** Task1 establishes immutable authored pairing and fail-closed scene/planner metadata. Task2 composes through the existing private Direct2D executor. Task3 proves original input; Task4 narrowly wires and stages the host. There is no second renderer or scheduler.

**Tech Stack:** Existing C++20/MSVC2022 x64, Direct2D/WARP, test-only pinned Telegram; Qt6.10.0/static `/MT` variant-none host.

**Spec:** `docs/superpowers/specs/2026-10-11-own-alpha-matte-design.md`.

Checkpoint 2026-10-11: Tasks1–3 complete. Task4 implementation/reviews and guarded
Release installation/local seed complete; installed native control acceptance
pending after two Windows capture timeouts. No repeated build is required to
obtain the user's Test Animation -> Party Popper manual receipt.

## Global Constraints

- Canonical E is `C:/Users/USER/Desktop/AveMotion`; H is `D:/rvc/c++/DragonianVoice/Avelabs-UI`. Work directly in main; scoped commits and ordinary push only.
- C++20, existing MSVC2022 x64/Qt6.10.0, existing dependencies; static host CRT `/MT`, own host variant `none`. No dependency installation, Windows/security/automation changes or new licensing/publication decisions.
- Preserve public backend fallback/skip policy, goldens, tolerances, vendor sources/notices, host threading/mailbox/docking/DPI/tray behavior and existing budgets.
- The previous private linear-gradient entry remains available with its existing scope. New capability uses a fixed private alpha-matte successor; public gradient/matte/blend support is not enabled.
- WARP is software; correctness and UI testing are not hardware-GPU or speedup evidence.
- Paired layers are root-adjacent shape layers, exactly one painted draw each; normal Alpha only, reciprocal stable identities, maximum64 pairs under existing128 layers. Unpaired existing scope remains unchanged.
- Original Party hash is EAB1FD7070BFFD2965C12A80D2D91CFF76683AF65019F142ACF9A553EE877B5C; input/artwork stays unchanged and local-evaluation-only, outside Git/Release.

## Review Focus

- A provider inactive or alpha0 must never expose unmasked content or mask artwork: Task1 timing/identity, Task2 pixel tests.
- Pairing cannot become a transform/opacity parent edge: Task1 provider-parent-to-content and opacity0 Null witnesses.
- Presentation opacity must be applied once, including0 and0.5, not divided by zero or squared: Task1 plan fields and Task2 literal pixels.
- A bad pair/resource/partial plan after a valid item must fail before native prefix drawing: Task2 preflight/failure/state tests.
- Bottom Difference over an opaque host must still use the transparent whole-frame backdrop: Task1 strict admission and Task2 nontransparent-host native witness.

## Task 1 Authored pairing and fail-closed scene metadata

**Files:**

- Modify `src/runtime/OwnVectorModel.hpp/.cpp` and `OwnVectorPropertyCompiler.hpp/.cpp` for bounded relationship/neutral metadata/Difference admission.
- Modify `include/avemotion/model/AssetModel.hpp`, relevant existing model hashing/validation code, `src/render/OwnSceneProgram.hpp/.cpp`, `src/render/OwnNativeEllipseStream.cpp`; a lowerer invalid-result guard propagates through `src/render/OwnNativeEllipsePreparedAsset.cpp` if required by review.
- Modify `include/avemotion/runtime/EvaluatedScene.hpp`, `src/runtime/RecordingBackend.cpp`, `include/avemotion/render/RenderPlan.hpp`, `src/render/RenderPlanner.cpp`, `src/render/HeadlessBackend.cpp`.
- Create `tests/support/OwnVectorMatteTestData.hpp` (owned shapes only); modify `tests/own_vector_model_tests.cpp`, `tests/own_vector_stream_tests.cpp` (including immutable model topology/resource assertions), `tests/render_plan_tests.cpp` and `tests/asset_validator_tests.cpp` for affected validator assertions. No backend/host edits in Task1.

**Interfaces:**

- Existing buildOwnVectorModel/validateOwnVectorModel/prepareOwnVectorAsset/lowerOwnVectorProgram/OwnNativeEllipseStream APIs remain signatures-compatible.
- OwnVectorLayerBinding adds typed SourceNodeId `matteSource`, `matteTarget`, bool `isolatedBottomDifference=false`; OwnSceneLayer adds uint32 `matteSourceLayer`, `matteTargetLayer` default `runtime::kInvalidSceneIndex`, same bool.
- SourceNodeRecord adds SourceNodeId `matteSource`; SourceBlendMode appends Difference without changing previous values.
- EvaluatedLayer adds uint32 `matteSourceLayer`, `matteTargetLayer` default kInvalidSceneIndex, bool isolatedBottomDifference=false. Content matte=Alpha, provider matte=None. Relationships are reciprocal, independent of parenting and remain on inactive layers.
- MotionRenderPlan adds float presentationOpacity=1 and bool presentationVisible=true, populated from PresentationState. Add RenderFeatureBlend=1U<<8U; both roles receive RenderFeatureMatte. Provider effectiveOpacity excludes common presentation opacity; content/unpaired retain existing behavior. Public unsupported accounting remains closed.

- [x] Write functional admission tests with a hand-authored root `[provider td1,content tt1]` pair (one fill each). Assert reciprocal source IDs, target Alpha, unchanged transform parents, exactly2 painted bindings. Add independent neutral layer ks.a/p/s `l:2` tests and bottommost root shape Difference source metadata assertions. Before production changes run the compiled model tests and retain expected feature-rejection RED, not compiler errors.
- [x] Add table-driven contextual rejection assertions: td/tt0/2/fractional/string/null/bool, orphan provider/content, consecutive roles/chains, non-shape/nested/precomp pairs, >one painted draw on either role, unknown fields and128-layer overflow. For `l`, reject1/3/fractional/string and group/paint/scalar contexts. For bm10 reject any authored layer below it, nested/precomp/null/paired roles, and other modes. Rejection publishes no prepared asset.
- [x] Implement validated authored sibling pairing without changing parent graphs or traversal clocks. Bind/source metadata, lower stable render identities/dependency bits and preserve default-only fingerprints. Allow only witnessed metadata contexts. All stored indices/ranges are validated without overflow; no tag-only claims or bitmap code.
- [x] Write stream/planner witnesses before their production edits: provider transforms parented to content; opacity0 Null transform parent not hiding a child; pair2->3 ip/op boundaries19/20/30/31; provider identity with empty draws; reverse/repeated seek and two rectangular viewports. Expected pair identities and matrices are literal, not recomputed by the code under test.
- [x] Implement reciprocal scene publication and topology hashing; mark both roles and isolated Difference unsupported publicly. Plan opacity test asserts provider inherited0.5 remains0.5 and content inherited0.5 under presentation0.5 becomes0.25; presentation0 produces empty plan, no divide-by-zero. Mutating only a valid pair relationship changes scene/plan topology while identical resources remain stable. Reject forged/nonreciprocal scene relationships before planning.
- [x] Run focused GREEN then fresh full three MSVC gates below once on frozen source bytes. Preserve existing goldens and report all warnings/failures. Self-review/report exact RED/GREEN commands/output, source/test hashes, limitations and changed files; do not claim Party raster or host acceptance. Review fix has fresh covering gates; earlier full95/96 are explicitly pre-fix, and combined final Task2/stage fullthree remain required per ledger ruling.
- [x] Product writer makes a scoped local commit only after fresh verification. Controller generates one task-scoped review package, independent review, resolves findings, STATE/ledger, and ordinary push after remote check. No push before review; fixes use new commits, never amendments/history rewrites.

## Task 2 Existing private Direct2D alpha composition

**Files:**

- Create `backends/direct2d/AlphaMatteBackendAccess.hpp`; modify `include/avemotion/backends/direct2d/Direct2DBackend.hpp`, `backends/direct2d/Direct2DBackend.cpp` and backend CMake linkage only if actual SDK needs it.
- Modify `tests/direct2d_contract_tests.cpp`, `tests/direct2d_contract/include/d2d1_1.h`, add fake `d2d1effects.h` only as native-boundary contract requires; modify `tests/own_vector_capture_tests.cpp`, reuse owned matte data and existing WarpCaptureSurface.

**Interfaces:**

- Consumes Task1 exact scene/plan relationship/presentation fields and existing canonical resources; preserves public Backend::draw and LinearGradientBackendAccess signatures/scope.
- Produces fixed private AlphaMatteBackendAccess::draw(Backend&, const MotionRenderPlan&, const RenderSession&) -> DrawResult. Internal fixed profile dispatch shares existing per-item geometry/solid/linear/stroke execution.

- [x] Add compiled functional private-capability RED for an owned valid pair and bottom Difference: initially the test dispatch seam invokes existing Backend::draw, asserting complete expected coverage; it fails on current unsupported skips. Once the private entry exists, switch only that seam to AlphaMatteBackendAccess with expectations unchanged. Compiler errors are not RED. Separately retain public skip of both pair roles/gradient/blend. Define interior literals: opaque red content ×50% provider =>premul RGBA(127/128,0,0,127/128); source RGB change has no effect; presentation0.5 scales content once, not twice. Require channel allowance<=3, no skipped admitted items.
- [x] Add whole-plan rejection before native resources/draws: valid prefix+bad reciprocal/provider/nonroot/multidraw pair, partial/reordered/duplicate item list, unsupported matte modes/mask/clip/dash/radial/other blend, corrupt canonical identities, invalid finite payloads/ranges. Extend fake contracts as needed; tests exercise backend behavior, not mere mock presence.
- [x] Implement fixed private preflight and shared primitive dispatch. With no pair/Difference marker, use direct shared dispatch and create zero command lists/effects (contract test). Otherwise record complete transparent frame and pair inputs on command lists; input0 content/input1 provider with DESTINATION_IN; suppressed providers never draw independently. Check native creation/Close/effect-property HRESULTs and restore target+transform with existing outer draw ownership unchanged. All captures are transient; no mutable effect caching or bitmap-mask renderer.
- [x] Add failure injection for each new native failure boundary, original target/transform restoration, context/factory/generation invalidation, inactive/alpha0 pair and public valid-prefix skip behavior. drawnItems means source primitives recorded, not visible host contributions; report that distinction.
- [x] Run real WARP API/state witnesses and native pixel tests: transparent/opaque host, lower unpaired artwork, half-alpha provider/content, animated geometry, source-parent-to-content, nonidentity host and presentation transforms, rectangular viewports, boundaries and reversed/repeated frames. Add fresh pinned CPU comparisons with unchanged existing capture policy; independent literal pair raster is mandatory. No self-backend oracle or own-field grafting.
- [x] Freeze source/tests, focused GREEN then all three full gates and both original Ducks. Independent task review, fixes/covering checks, STATE/ledger, scoped commit/push. No host replacement yet. Review fix has fresh full no-ref/allthree contracts and native alpha coverage on identical final backend/capture bytes; prior full95/96/Ducks are explicitly pre-fix. Final combined stage fullthree remains mandatory before Release per ledger rulings.

## Task 3 Unchanged original Party acceptance

**Files:** Modify only existing external scene/capture test seams and owned matte-specific oracle helper if needed; add `docs/OWN_ALPHA_MATTE_2026-10-11.md` evidence report. Raw original assets/results stay under ignored `out/`.

**Interfaces:** Consumes Task1 immutable original admission and Task2 fixed private raster entry. Produces unchanged-input correctness report usable by Task4; no new feature support in this task without a recorded design correction and regression RED.

- [x] Rehash unchanged Party and both Ducks; require original Party hash/size/metadata from spec. Run all92 Party frames forward/reverse/repeat at129x129 and192x96, with literal four pair identities/intervals and fresh ordinary scene fields where available. Preserve ordinary missing-pair/local-field limitations in report; do not call them full-field equality.
- [x] Capture Party0,19,20,25,30,31,50,81,82,91 at129x129 and192x96; compare fresh pinned CPU and validated independent native/literal evidence, zero skipped own items. Export tightly packed premultiplied CPU BGRA for129-square frame0 with original/hash/pin/build provenance for the host gate, not the public solid-only WARP reference or an alpha-discarding PPM. Keep raw failures, exact thresholds and input bytes. Run original Duck model/full-scene/capture regressions using established commands.
- [x] Apply the spec's precision execution addendum: original full images use separate independent-native and fresh CPU decisions under the unchanged PixelComparisonPolicy; owned/literal interiors remain <=3. Retain the failed provisional whole-image <=3 evidence, exact independent float32 parenthesization/matrix-only raster witness, all raw maxima and explicit non-pixel-exact qualification. Do not change production transforms or any existing threshold/golden.
- [x] Any failure: reproduce/root cause first, narrow design addendum and new regression RED before production fix; no stripped Party, golden/tolerance changes or false acceptance. Task-specific independent review and report/STATE/ledger commit/push. Task4 is blocked until this acceptance is real. Product/report reviewed; controller checkpoint and ordinary publication follow the ledger.

## Task 4 Narrow Avelabs wiring and guarded Release delivery

**Files:** H `src/app/motionlab/OwnMotionRenderer.cpp`, `cmake/AveMotionLab.cmake`, existing `tests/motionlab/tst_own_motion_renderer.cpp` and catalog original seam/tests only if needed, host testing/report docs; local Party catalog entry/file outside Git/Release.

**Interfaces:** Consumes reviewed original acceptance and AlphaMatteBackendAccess; returns existing owned premultiplied QImage path. Worker/canvas/playback APIs and scheduling remain unchanged.

- [x] Read H maintenance/roadmap and current delivery provenance. Test-first original Party renderer acceptance using a separate opt-in external Party frame-zero slot and Task3 fresh CPU BGRA under the unchanged engine PixelComparison default policy. Keep the existing Duck external slot, exact-byte fit oracle and its one-byte-negative assertion intact; do not reinterpret it as a tolerance test. Retain generic JSON/TGS/unsupported errors, owned buffer/lifetime, fit/transparency and zero skipped-item checks. Watch functional RED before renderer/CMake edits. The CPU frame-zero host gate proves one-frame independent parity, not mask coverage by itself; Task2/3 own the kernel and multi-frame evidence.
- [x] Wire the fixed private entry with target-private include path and strict feature validation; never blanket-ignore unsupportedFeatureItemCount. Fresh own/reference host harnesses verify renderer, transport/hidden lifetime/replacement,1/4/16 and transparent parent.
- [x] Add local-evaluation-only pinned Party via contained/hash-checked catalog; preserve prior saved/default selection. Do not commit artwork. Original Party worker/page gates use generic92-frame metadata; do not use Duck179-frame boundary environment.
- [x] Stage a fresh variant-none `/MT` candidate through existing static build and Runtime-only install path; verify native smoke/original/tests/imports/readiness/SymChk and exact source/build provenance. Avoid Symbols install's absolute canonical destination until guarded promotion. Preserve current EXE/PDB until verified candidate and zero installed processes.
- [x] Independent host task review and strongest combined-stage review of frozen packages/reports. Fix all Critical/Important findings with covering tests; maintain scoped commits/ordinary pushes in each repository after remote checks.
- [x] Back up26-file Release plus matching PDB, guarded promotion, installed hashes/provenance and launch smoke. Full package/PDB and installed readiness/SymChk verified; actual launch returned the own window. Matched Release performance remains a separate stage; no unmeasured optimization/speedup claims.
- [ ] Native original playback/pause/seek/restart/1/4/16/transparent backdrop validation. Two capture timeouts prevented observation after refreshed selection/activation; user check requested. No UI/API-harness substitution, alternate manual/DPI/GPU acceptance or fake completion. Do not rebuild/reimplement solely for this pending receipt.
- [x] Finish report/STATE/ledger with E/H build SHAs, commands/counts/native/CPU/UI coverage, retained warning/raw limitations, every ruling and costs, installed provenance and next step. Delivery handoff explicitly leaves native user acceptance and later measurements open. Retain recovery/evidence artifacts; no automation changes.

## Verification commands

```powershell
& 'C:/Program Files (x86)/Microsoft Visual Studio/2022/BuildTools/Common7/Tools/Launch-VsDevShell.ps1' -Arch amd64 -HostArch amd64 -SkipAutomaticLocation
cmake --build out/build/windows-msvc-direct2d --config Debug --parallel 4
ctest --test-dir out/build/windows-msvc-direct2d -C Debug --output-on-failure --no-tests=error -j 1
cmake --build out/build/windows-msvc-direct2d-capture --config Debug --parallel 4
ctest --test-dir out/build/windows-msvc-direct2d-capture -C Debug --output-on-failure --no-tests=error -j 1
cmake --build out/build/windows-msvc-win32-preview --config Debug --parallel 4
ctest --test-dir out/build/windows-msvc-win32-preview -C Debug --output-on-failure --no-tests=error -j 1
```

Enumerate actual registered focused test names before invocation. Existing H
delivery command records: `build/checks/2026-10-08-test-animation-001/delivery/*.command.json`.
Use these established commands with fresh named candidate paths, not unchecked
`build_ui.bat` installation. Test totals are observed, never historical assumptions.
