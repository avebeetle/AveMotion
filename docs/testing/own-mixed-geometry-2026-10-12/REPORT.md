# Own mixed geometry — 2026-10-12

Status: implementation committed; validation and checked UI delivery in progress.
Installed Avelabs `build/Release` is still the previous verified package.

## Result and limits

Commit `e0949a2e81b8036e2b6d9e7f68b56758d1f66099` lets the existing public own
playback combine contour, ellipse and rectangle sibling groups. It reuses the
canonical model, evaluator, scene program and existing primitive generators.
New primitive groups are exactly `[el|rc, fl, tr]` in root shape layers; this is
not full Lottie support. Existing path-only and legacy primitive-only profiles
remain separate, with explicit unsupported-input errors and no compiler retry.
The public interface, Player, renderer ownership and host UI behavior are unchanged.

The first-party `tests/fixtures/own/mixed_geometry.json` contains three authored
groups and 61 frames. Public regressions cover literal geometry/order, positive
fractional sizes, primitive property animation, independent playback, retained
scenes and unsupported scopes. No external artwork or new upstream code was added.

## Completed checks

- Functional public/model/stream RED preceded production, then focused3/3 and
  full reference-none45/45 passed. Independent Task1 review: no findings.
- Fresh reference-none install package46/46, package-boundary check and separate
  installed public consumer configure/build/run passed. The local package has no
  reference renderer; this is not permission to redistribute derived code.
- Task2 full model and stream differential tests passed. Focused mixed capture
  produced455 byte-exact own/ordinary-WARP samples:453 CPU-policy passes and
  two explicitly guarded expected-negative CPU rejections below. Three
  missing/moved/recolored mutation witnesses are caught; no policy was relaxed.
- Separate Qt test build passed mixed-page load/Play/Pause/seek,1/4/16 images
  and resize, using independent133x100/100x75 expectations. Final controlled
  icon DPR1/1.5/2 runs each passed3 QtTest checks (including lifecycle), with
  physical20/30/40px output. The38 final PNGs show transparent composition and
  state changes; they are not physical-monitor or live-pacing acceptance.
  Fresh full platform/host gates, original-asset measurements and delivery remain pending.
- Independent Task2 spec/quality review approved the test-only slice with no
  findings, independently checking all455 WARP pairs and literal CPU witnesses.
  Full stage/delivery and physical-monitor acceptance were not inferred from it.

The initial45-test run was44/45 because an unchanged child Ninja configure
could not find `cl`. Command-scoped VsDevCmd recovered it; original failure logs
were retained and no product/CMake workaround was added.

## Subpixel comparison boundary

At512x256/frame60, the0.5x0.25 ellipse has four CPU alpha values7,7,6,6 versus
own/ordinary-WARP9,9,11,11. The unchanged active threshold is alpha>8, so CPU
omits only this ellipse from the global active bounds: delta81 versus limit4.
The other five policy criteria pass; all524288 own/ordinary-WARP raw bytes match.
CPU compared with independent ordinary-WARP fails identically. Independent audit
confirmed this is a cross-raster threshold boundary, not an own geometry mismatch.

The approved design addendum requires a narrowly named expected-negative test:
assert the sole policy rejection, all other criteria, exact independent WARP
and literal nonzero/color/position witness. Keep `cpu_pass=0`, the original failed
receipts and all authored inputs/samples. Do not describe it as CPU-policy PASS.
No global tolerance, golden, rasterizer or asset change is permitted for this case.

## Evidence and environment

Engine raw receipts: `out/checks/2026-10-12-own-mixed-geometry-001`.
Host raw receipts: `D:/rvc/c++/DragonianVoice/Avelabs-UI/build/checks/2026-10-12-own-mixed-geometry-001`.
C disk exhaustion interrupted a later capture run; the actual failed LastTest
and unreliable Tee-masked shell0 are retained, not counted as success. Root moved
only this stage's7892 capture files (2125847079 bytes) to host `task2/engine-captures`,
with per-file size/SHA256 verification. Historical log paths are not rewritten;
`task2/capture-relocation-before.json` and `capture-relocation-after.json` bind the
original/destination inventory. Old stage evidence and installed binaries remain.

Protected installed EXE SHA256 is `17AA33DB491217401C5BDC7F73F1D0C19086AF80FC130FCBEA04D5C8C8ABDC41`;
PDB is `B2DAB8CB36D733F8A6532D9005487D081BA64073EEC74EC9325E957E81529C70`.
Those are prior compiled E92e1cfa/Haa7474e, not this stage's revisions.

## Remaining result

Finish focused capture/UI evidence and independent review, then fresh full
Win32-preview/host gates, the saved original-asset measurement scenarios and
protected static /MT delivery. No new scheduler/cache/GPU/docking/physics work.
Automated snapshots and controlled Qt scales cannot close live physical-monitor
appearance or mixed-monitor DPI acceptance; the user currently has no PC access.
No performance or memory improvement is claimed by this functionality stage.
