# Own mixed geometry implementation plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Combine supported ellipse, rectangle and contour groups through public own playback, then verify the unchanged UI route.

**Architecture:** Extend existing canonical vector admission and lower primitive property bindings into the existing scene program. Select the compiler once; leave primitive-only legacy behavior and the public interface unchanged.

**Tech Stack:** C++20, MSVC, CMake/CTest, existing evaluator/Player, Direct2D/WARP, Qt Widgets 6.10.0 static /MT.

**Spec:** `docs/superpowers/specs/2026-10-12-own-mixed-geometry-design.md`

## Global Constraints

- No compiler retry or reference fallback; unsupported input remains an explicit error.
- New primitive groups contain exactly `[el|rc, fl, tr]` in root shape layers; no primitive trim/stroke/gradient/outer paint, nesting, precomp or matte.
- `p` components in `[-32768,32768]`; `s` components in `(0,16384]`; rectangle `r` in `[0,16384]`; integer `d` is `1` or `3`.
- Preserve legacy primitive-only grammar/fitting, existing path-only support, transparent premultiplied output, independent playback and immutable published data.
- No new scheduler, per-icon thread, large cache, resource manager, GPU route, dependency installation, license change or docking/physics changes.
- Main in place and scoped ordinary push are delegated; preserve unrelated edits, evidence, vendor sources and notices. Root owns Git/checkpoints and delivery; one product implementer at a time.

## Review Focus

- Fractional size and zero/negative/over-bound animated keys: enforce strict lower bound, not `low=1`.
- Unsupported primitive scope: reject atomically without changing path-only admission.
- Animation of primitive radius/size/position: classify and refresh geometry, leave unrelated static resources unchanged.
- Equal content and repeated seeks: retain source identity and immutable scenes across independent instances.
- Legacy grammar and aspect fitting: document selection must not silently move old primitive assets to vector semantics.

---

### Task 1: Canonical mixed admission and public playback

**Files:**
- Modify: `src/runtime/OwnVectorModel.cpp`, `src/runtime/OwnVectorPropertyCompiler.cpp`, `src/render/OwnSceneProgram.cpp`, `src/render/OwnNativeEllipsePreparedAsset.cpp`.
- Test: `tests/own_public_playback_tests.cpp`, `tests/own_vector_model_tests.cpp`, `tests/own_vector_stream_tests.cpp`, `tests/support/OwnVectorTestData.hpp` only if shared test data is useful.
- Create: `tests/fixtures/own/mixed_geometry.json` (first-party small composition, no external artwork).

**Interfaces:**
- Consumes: `buildOwnVectorModel`, `validateOwnVectorModel`, `lowerOwnVectorProgram`, `materializeOwnScenePath`, existing property semantics and public `prepareOwnAsset`/`createOwnPlayback`/`evaluateAt`.
- Produces: same signatures; mixed input accepted by the existing prepared vector owner, with three authored geometry kinds and existing scene resources.

- [ ] Step 1: Add public `mixedGeometryUsesCanonicalPlayback` using sibling el/rc/sh groups; assert three draws in reverse authored order, literal path bounds, static resources and preserved source identity. Add independent playback/retained scene and animated p/s/r/hold/out-of-order seek assertions; reuse the existing legacy fixture for unchanged fitting and animation.
- [ ] Step 2: Build/run `avemotion_own_public_playback_tests` before production changes. Save RED output showing mixed preparation fails for missing capability, not a build error.
- [ ] Step 3: Add contextual invalid-input tests for direction2, size0/negative/>16384, negative radius, malformed/unsupported tracks, prohibited scopes and float-collapsed static geometry. Prove positive fractional sizes are accepted after implementation.
- [ ] Step 4: Extend model admission and validation with existing Ellipse/Rectangle kinds and property semantics. Propagate Vec2 strictLow. Bind p/s/r in lowering and geometry classification. Select vector on any sh, legacy primitive only otherwise. Reject collapsed static primitive resources during vector preparation. Do not edit the path generator, stream, public API or backend.
- [ ] Step 5: Run public/model/stream tests, then the full existing reference-none CTest suite once; preserve all logs and names of any failures. Self-review the scoped diff and report RED/GREEN commands, counts, files and concerns.
- [ ] Step 6: Root obtains independent task spec/quality review, handles fixes, verifies focused tests, updates STATE/ledger and makes a scoped commit and ordinary push after ancestry check.

### Task 2: Differential, UI evidence and checked delivery

**Files:**
- Test: existing `tests/own_vector_model_differential_tests.cpp`, `tests/own_vector_stream_differential_tests.cpp`, `tests/own_vector_capture_tests.cpp` as needed for mixed cases.
- Host test: `D:/rvc/c++/DragonianVoice/Avelabs-UI/tests/motionlab/tst_motion_page.cpp` only for a small optional icon PNG evidence hook if needed.
- Report/checkpoint: `docs/superpowers/STATE.md`, a concise stage report under `docs/testing/`, Avelabs `docs/ROADMAP.md` and existing testing documentation.

**Interfaces:**
- Consumes: Task1's unchanged public playback and the first-party mixed fixture; existing fresh ordinary oracle/capture, Test Animation external-fixture route and frozen measurements.
- Produces: reproducible captured-image/geometry/regression evidence and a checked static reference-none UI build, not a new engine/API or diagnostic system.

- [ ] Step 1: Add mixed static/animated/direction/roundness/fractional cases to the existing differential/capture tests with unchanged policies. Assert structural/reference geometry and visible transparent output, not only successful preparation.
- [ ] Step 2: Run full `windows-msvc-win32-preview` gates and public installed consumer against a fresh reference-none package. Preserve raw logs, configure/build/source provenance and selected output images. Investigate failures; never change goldens to pass.
- [ ] Step 3: Rebuild separate Avelabs check targets against the new source. Run existing own UI suite plus original Duck/Party external-fixture regressions and mixed fixture at existing 1/4/16 controls. Save existing page screenshots; inspect pixels/images. If adding icon PNG hooks, use existing realTtsIconUsesAsyncOwnedFrames rest/hover/press/rest checkpoints and controlled scale invocations; no screenshot subsystem.
- [ ] Step 4: Run the existing saved load/steady/pause/hidden measurement scenarios for original assets with matched physical size and provenance. Report limits and differences without requiring or claiming speedup. Do not reconfigure or overwrite frozen baselines.
- [ ] Step 5: Independent task review and whole-stage review, fresh covering gates, scoped commits/push. Promote only after existing protected EXE/PDB backup and non-running-process checks. Record compiled SHAs separately from report commits. Leave native mixed-monitor visual acceptance open.

## Approval and self review

Delegated execution choice: subagent-driven, one writer at a time, root owns Git
and protected delivery. Tasks1/2 share only the unchanged playback contract and
fixture; Task2 validates Task1 without expanding its format. Each review-focus
entry has Task1 regression coverage. The full stage remains incomplete until
Task2 evidence and checked delivery are finished.
