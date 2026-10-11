# Task3 unchanged original Party acceptance

Date2026-10-11; canonical E `C:/Users/USER/Desktop/AveMotion`; base `c415df7ed15e4cfe1884a23f74796d2fa85e17ef`. Original acceptance checks PASS under the written spec precision execution addendum; final combined suites/integration receipt below. Historical NEEDS_CONTEXT was resolved by controller acceptance-design adjudication, not a production numeric fix. No production, host, vendor, dependency, Windows/security/automation/license/publication or existing tolerance edits. No push/amend/reset/subagents. Controller-owned STATE/spec/plan edits preserved and excluded from this implementer's scoped commit.

## Scoped implementation and literal/oracle coverage

Changed test seams: `tests/own_vector_stream_differential_tests.cpp`, `tests/own_vector_capture_tests.cpp`; narrow helpers: `tests/support/OwnVectorSceneOracle.hpp`, `tests/support/OwnVectorSceneComparison.hpp`, `tests/support/OwnVectorMatteTestData.hpp`; public report `docs/OWN_ALPHA_MATTE_2026-10-11.md`. New CLI `--party-original <absolute-original-json>` bypasses TGS decoding only for the original JSON test entry; normal public/Duck routes are unchanged.

Literal facts are independent authored IDs/intervals: provider/content2/3 on[20,31);4/5,6/7,8/9 on[20,82); providers all[20,82). Asserts reciprocal own model and scene bindings, two-role dependency flags, content Alpha/provider None, exactly8 roles, exactly1 authored Difference marker on bottom popper11. Checks original byte length/metadata/11authored layers/12scene layers. Shell SHA256 checks supply original-byte integrity.

Fresh ordinary finalized model/scene source from `NativeEllipseOracle::freshScene` remains separate from own preparation. `OwnVectorSceneOracle::freshScene(...,true)` checks original-specific mode/root/masks/clips and retains INVALID reciprocal indices/no isolated Difference in raw reference. It derives matrices through `sourceWorld` on its independently cache-disabled parsed pinned source; it does not read own matrices. This is necessary because pinned `LOTCompLayerItem::renderList` suppresses the provider when matte content is inactive, while `renderTree` exports the still-visible provider (Party3 ends31; provider2 ends82). Default `assertCanvasClip` is unchanged and still rejects mattes; new original matte comparison uses only its narrow assertion. Existing solid RGBA/local-solid/local-opacity strictness is unchanged; final gradient parity keeps the existing explicit missing-local/separated field qualification.

Scene gate: all92frames ×forward/reverse/repeat ×129x129/192x96 =552samples. Available visible fields checked: ordered source roles/clock/ordinals; path binding/modifier flags; exact verbs/point counts and final/predicted-local bounds/points; independently parsed matrix vs ordinary and own; fill rule; solid/final-gradient stops+endpoints; existing stroke/miter behavior; available local solid paint/local stroke; visible layer matte mode/mask/clip/opacity. Maximum path/matrix/final-gradient numeric1.52588e-05 <=existing1e-4. Predicted-local max0. Ordinary missing reciprocal identity/isolatedDifference/local-gradient/separated-gradient fields are NOT full-field parity.

Native helper `ordinaryPartyDraw` draws original ordinary final or local path, raw final paint, independent parsed paint matrix. Gradient inverse-mapped endpoints and transformed brush retain nonuniform paint metric; no own resources/model/plan/backend are used for accepted expected pixels. `ordinaryPartyRaster` groups raw native paints by authored role and uses independent integer premultiplied multiply/source-over plus literal pairs; it suppresses providers and inactive content. It is a test-only per-input raster/compositing oracle, not native Composite-effect graph proof (Task2 covers that). Final optional matrix override consumes only independently audited literal general-affine matrices for the precision witness, never own scene fields. Earlier explicitly diagnostic own-matrix override experiments remain in raw archived artifacts, not final oracle code. Raw `*-ordinary.tsv` and `*-local-ordinary.tsv` export selected visible raw fields: authored roles, visibility/matte/opacity, final path points, solid RGBA, final gradient endpoints/stops. These are selected raw ordinary field exports, not complete serialized scenes.

## TDD chronology / exact same-shell command pattern

Every build/test shell initializes:
```powershell
Import-Module 'C:/Program Files (x86)/Microsoft Visual Studio/2022/BuildTools/Common7/Tools/Microsoft.VisualStudio.DevShell.dll'
Enter-VsDevShell -VsInstallPath 'C:/Program Files (x86)/Microsoft Visual Studio/2022/BuildTools' -SkipAutomaticLocation -DevCmdArguments '-arch=x64 -host_arch=x64'
```
Raw root `out/own-alpha-matte-task3`. Every native command stdout/stderr uses `2>&1 | Tee-Object <raw-log>`, `$nativeCode=$LASTEXITCODE`; build prints BUILD_NATIVE_EXIT and nonzero stops shell; test prints TEST_NATIVE_EXIT/WRAPPER_EXIT then `exit $nativeCode`. Tee logs contain raw native output, subsequent wrapper labels were captured from tool shell output. First exploratory RED shell ran both RED tests and exited0 after recording each native1; this wrapper exception is explicit below.

Initial functional RED used both new original CLI seams with the existing strict `run` functions; build target pair:
```powershell
cmake --build out/build/windows-msvc-direct2d-capture --config Debug --target avemotion_own_vector_stream_differential_tests avemotion_own_vector_capture_tests --parallel 4
& './out/build/windows-msvc-direct2d-capture/Debug/avemotion_own_vector_stream_differential_tests.exe' --party-original C:/Users/USER/Desktop/AveMotion/out/real-corpus-2026-10-08/assets/noto-1f389.json
& './out/build/windows-msvc-direct2d-capture/Debug/avemotion_own_vector_capture_tests.exe' --party-original C:/Users/USER/Desktop/AveMotion/out/real-corpus-2026-10-08/assets/noto-1f389.json
```
`red-build.log` build0; `red-stream_differential.log`, `red-capture.log`: native1 each, `ordinary matte/mask absent`. Shell wrapper0 deliberately observed both failures; capture root inherited from environment `out/part26n/task3/captures/run-171800978518500`. This is functional strict-oracle rejection, not compiler error.

Original helper first scene iteration build0/test1 (`scene-build.log`, `scene-test.log`): `matrix role bijection covers every visible draw`. Rootcause renderList provider omission above; no production fix. Narrow independent parsed-source matrix helper then build0/test0/wrapper0 (`scene-build2.log`, `scene-test2.log`),552samples.

Capture functional RED used new native/CPU oracle but original own draw still public `renderPlan`. `capture-red-build.log` build0, test1/wrapper1 (`capture-public-red.log`) first exposed test-helper one-paint assertion on unpaired6-paint popper. Corrected helper to enforce one paint ONLY for paired roles, while grouping unpaired raw paints in original order; second RED build0, test1/wrapper1 (`capture-red-build2.log`, `capture-public-red2.log`): `WARP supported draw coverage`, expected public skip of isolated Difference/gradient. Original own dispatch then switched ONLY to reviewed fixed private `renderAlphaPlan`; no new production behavior.

Commands for each capture iteration (build target always the same):
```powershell
$env:AVEMOTION_TASK_CAPTURE_ROOT='C:/Users/USER/Desktop/AveMotion/out/own-alpha-matte-task3/<capture-root>'
cmake --build out/build/windows-msvc-direct2d-capture --config Debug --target avemotion_own_vector_capture_tests --parallel 4
& './out/build/windows-msvc-direct2d-capture/Debug/avemotion_own_vector_capture_tests.exe' --party-original C:/Users/USER/Desktop/AveMotion/out/real-corpus-2026-10-08/assets/noto-1f389.json
```
Roots/logs respectively `public-red-captures`/capture-public-red{,2}; `green-captures`/capture-green; `diagnostic-captures`/capture-diagnostic; `diagnostic2-captures`/capture-diagnostic2; `diagnostic3-captures`/capture-diagnostic3. Each corresponding `*-build.log` native0. Initial alpha and all3diagnostic native tests/wrappers1: `original Party capture failures=6`. At that historical point capture/native GREEN was not achieved; the later written acceptance-design correction and final decisions are recorded below, not a production fix.

## Original raster results and concrete failure

All20 captures CPU PASS unchanged policy; zero skips, source drawn counts perviewport frames0/19/20/25/30/31/50/81/82/91 =6/6/24/24/24/23/23/13/6/6, total310source primitives across20. Native restores host target and identity transform via existing native helper.

Unchanged image policy: active alpha threshold8; IoU>=0.78; alpha relative error<=0.12; mean all<=12; mean active<=30; fraction of active-union pixels with channel delta>64 <=0.18; bounds delta<=4. Observed CPU minIoU0.989197; maxalpha0.0196096; maxmeanAll2.09738; maxmeanActive5.0699; maxlargeFraction0.000245519; maxbounds1. Exact perframe metrics retained. Full-native maximum<=3 was an additional provisional Task3 check, subsequently adjudicated by the controller's written execution addendum below; no silent implementer relaxation. Literal/kernel/interior3 policy from Task2 untouched.

129square native full-image max0 except frame31max1. Rect192x96 frames0,25,50,81,82,91 max20;19max1;20,30,31max0. First failing unchanged root `green-captures/run-172142896372300`. Reproduced roots `diagnostic-captures/run-172226790245500`, `diagnostic2-captures/run-172286695605500`, `diagnostic3-captures/run-172324162703900` retain perframe CPU/own/native/native-local BGRA and diff/white PPM plus selected ordinary fields. PPM is for inspection only, never alpha correctness.

Rootcause experiment:192x96frame0 no active mattes; draw0 original popper shapes/5 yellow solid. Vendor raw/independently parsed dy63.6337547302 vs own dy63.6337585449, delta3.8147e-6. Final-space and vendor-local native constructions both max20. Holding the same ordinary local geometry/color constant and changing ONLY native transform to own's diagnostic matrix reproduces isolated max20; other5draw matrix-only probes max0. Ten isolated pixels exceed3, seven after ordinary overdraw; worstx68,y88 expectedBGRA1,39,52,52, actual2,54,72,72. This establishes native curved-edge coordinate sensitivity, not an unproven internal Direct2D tessellation mechanism, pair-math defect, provider leakage or nonuniform gradient defect. No production numeric fix or original-specific adjustment attempted.

Evidence SHA256: capture-diagnostic3.log `3EA0943F0600B265D184804BC55F5809C2A1CFA80D994962ACCEBB7DB77DB177`; diagnostic3 party-metrics.tsv `9E6C43E687F1073AB568835F08DD32F6FB0CD0428D1CAC3A4C20B1D900690633`; isolated draw0 reference `3ABFA0BB4E29C24E9C4E24C4AF01F9ED2AC361344E4DADB2408FE470C77282D6`; matrix-only `427736EE483A4CFC986F91D163F8CB643537417AEB43EB3AE6D407F90A5A56F2`.

## Original hashes / fresh CPU host artifact provenance

Party SHA256 `EAB1FD7070BFFD2965C12A80D2D91CFF76683AF65019F142ACF9A553EE877B5C`,67875bytes. a-2.tgs `2CDBF4D03DE421B6E0BC73871F4FC7ABB685EF3B3B30B581E8AB75C71501B15A`; a-3.tgs `7CD55718288EB1B1D1EDFD0E774D9038E076A7F5AADF9835A92CF942948A4BC1`, original local files only.

Fresh pinned CPU artifact: `out/own-alpha-matte-task3/diagnostic3-captures/run-172324162703900/party-129x129-f0-cpu.bgra`,66564bytes,129x129,stride516,tightly packed premultiplied BGRA8 SHA256 `03593C7BC15E459394601A03214154DEAA8D46763DB6AB683CD0FFD5157F699A`. Adjacent provenance markdown records full source/pin/cache/executable hashes and exact command. It comes from cache-disabled fresh rlottie parse and ordinaryCpuPixels: zero-initialized uint32 buffer, renderSync default keepAspectRatio=true, explicit pixelFromArgb32 conversion; same CPU semantics/layout as ReferenceAnimation::renderFrame(0,129,129,true), without adding ReferenceRuntime link. No white/background/alpha stripping, unrelated to prior public solid-only reference.

Pinned Telegram `67f103bc8b625f2a4a9e94f1d8c7bd84c5a08d1d`, existing metadata/build patches perUPSTREAM; existing Debug rlottie.lib SHA256 `BBCACF04DFF386CDAAE64135A5EED6CF9BAD7692DBB654FA8C2CE75A46EC56A8`; diagnostic3 capture.exe `800347180F633EB94374E9F7DEFB3646388C40015F653F188D90895895616CBA`; CMakeCache `F1B3257A98C01F7208C01833371E86F236FCC92BF0ABCEC9FE447F05414119B6`. MSVC2022x64 Debug VS17.14.41/MSBuild17.14.60; existing incremental cache, not clean/Release/hardware proof.

## Verification, warnings and remaining work

Fresh full no-reference command after same-shell prefix:
```powershell
cmake --build out/build/windows-msvc-direct2d --config Debug --parallel 4
ctest --test-dir out/build/windows-msvc-direct2d -C Debug --output-on-failure --no-tests=error -j 1
```
First build native1/wrapper1 (`full-no-reference-build.log`) caught helper include boundary: own_vector_model target does not have src/render include path; tests were not run after build failure. Replaced concrete Prepared helper include with dependent template argument, preserving assertions; second native build0/CTest0/wrapper0,47/47 in11.23sec, `full-no-reference-build2.log`, `full-no-reference-tests2.log`. No failures/skips in that suite.

Original Duck model/stream/full-differential/capture runs completed: all8native executable exits0 with captured per-executable wrapper0 and containing shell0. Exact established CLI after same-shell prefix:
```powershell
$env:AVEMOTION_TASK_CAPTURE_ROOT='C:/Users/USER/Desktop/AveMotion/out/own-alpha-matte-task3/duck-captures'
foreach($duck in @('a-2','a-3')) {
 $inputPath='C:/Users/USER/Desktop/AveMotion/out/real-stickers-2026-09-28/assets/'+$duck+'.tgs'
 foreach($suite in @('model','stream','stream_differential','capture')) {
  $exe='./out/build/windows-msvc-direct2d-capture/Debug/avemotion_own_vector_'+$suite+'_tests.exe'
  $nativeArgs=@('--tgs',$inputPath)
  if($suite -eq 'stream_differential'){$nativeArgs+='--full'}
  & $exe @nativeArgs 2>&1 | Tee-Object ('out/own-alpha-matte-task3/duck-'+$duck+'-'+$suite+'.log')
  $nativeCode=$LASTEXITCODE
  Write-Output ('DUCK='+$duck+' SUITE='+$suite+' NATIVE_EXIT='+$nativeCode+' WRAPPER_EXIT='+$nativeCode)
  if($nativeCode -ne 0){exit $nativeCode}
 }
}
exit 0
```
Original a-2 inventory50layers/52draws/94tracks; forward/reverse all180frames128/129/256/512square; full ordinary2960samples;68pixel-exact native captures/CPU policy PASS/skips0 at `duck-captures/run-172478366905700`. Original a-3 inventory37layers/36draws/33tracks; existing512x256 admission and256x512 rejection retained; full ordinary2960samples;68pixel-exact native captures/CPU policy PASS/skips0 at `duck-captures/run-172660366583800`. Original capture paths stay public. Retained precomp clipping witnesses pass. Raw local diagnostics unchanged: a-2 over-limit239805(max33.6390705109),a-3 over-limit23115(max0.0001220703125); qualified final-path/parsed residual comparisons pass, not raw local equality claims. a-2 finalpath/matrix max1.52588e-05,stroke2.47955e-05; a-3 finalpath3.05176e-05,matrix2.28882e-05,stroke1.26362e-05.

Three combined full final gates were deferred during iteration, then executed here on the controller's explicit request after final source freeze (results below); controller will not duplicate unchanged runs. InitialTask3 modified test builds emit inherited rlottie.hC4251 (retained raw, not suppressed); LF/CRLF Git warnings retained. Git operations use per-command `-c safe.directory=C:/Users/USER/Desktop/AveMotion`, no global config write. Final `git diff --check`0.

Self-review: untouched strict default routes; fresh ordinary missing-field qualifiers intact; final precision witness uses literal matrices, no own fields as an oracle. Capture file is already large and grows~220lines in established seam; no unplanned file decomposition. No unsupported content stripping, existing golden/policy changes or production feature expansion. Original full-image native residual remains explicitly qualified; future exact float32 association correction requires separate coordinated geometry/paint/clip design. Final scoped commit/review receipt below.

## Controller adjudication and independent precision regression

Controller written execution addendum read verbatim from `docs/superpowers/specs/2026-10-11-own-alpha-matte-design.md`, amended Task3 checklist read from `docs/superpowers/plans/2026-10-11-own-alpha-matte.md`. The provisional arbitrary original full-image max<=3 was a new stronger instrument, not the spec's literal/kernel/interior<=3 rule. Controller approves separate original independent-native and freshCPU full-image decisions under unchanged defaultPixelComparisonPolicy. Existing tolerances/goldens and owned/interior3 checks remain intact. This is an explicit acceptance-design correction; the failing whole-image3 runs are preserved, not a claimed production TDD RED/GREEN fix.

Audit traces own world-before-viewport association via `src/evaluation/PropertyEvaluator.cpp` and `src/render/NativeEllipseEvaluationHelpers.hpp::nativeEllipseViewportTransform`; vendor applies viewport to layer before shape-group multiplication in `third_party/rlottie/telegram/source/src/lottie/lottieitem.cpp`. Independent general-affine authored L/P/G/V float32 operands reproduce final own `(G*(L*P))*V` dy63.633758544921875 versus vendor `G*((L*P)*V)`63.63375473022461. This sufficient numeric reproduction DOES NOT prove each actual intermediate vendor/own coefficient bit-identical. An attempted VMatrix world-first test produced63.6337623596 (an additionalULP), owing to optimized type/multiplication branches; it was rejected as an incorrect probe and replaced by explicit independent general-affine per-product/per-sum float32 rounding. `literal-precision{,2}-build.log` native0; both native tests/wrappers1 at literal association values, archived intermediate test-helper failures.

Final `originalPartyPrecisionWitness` constructs independently audited literal L/P/G/V operands, asserts both final dy literals, asserts onlydy changes, draws unchanged ordinary-local path/color under both literal matrices, and asserts vendor-literal native match<=3 plus sensitivity>3. No own scene/plan/model/backend fields feed either expected operand. Final result vendor_native_max0/sensitivity_max20; raw literal matrices' rasters retained. Hand-checked original yellow opaque interiors at129(42,51),192x96(72,44) assert authoredBGRA6,193,255,255 within3, plus literal transparent corner, on own and independent-native rasters.

First final owning original acceptance (pre-single-dy-assertion addition, same pixel construction) same-shell prefix/build target pair and externalCLI commands: `final-original-telegram-build.log` native0; `final-original-telegram-stream_differential.log`0/wrapper0,552samples; `final-original-telegram-capture.log`0/wrapper0,20native+20CPU decisions and four original interior/corner checks. Raw root `final-original-telegram-captures/run-173107003482500`. Final code freezes the added onlydy assertion before the combined suites; final owning original repeats are appended with those exact bytes.

Legacy focused owning checks before final-only witness/decision code freeze: fullTelegram cache build0, `ctest --test-dir out/build/windows-msvc-direct2d-capture -C Debug --output-on-failure --no-tests=error -j 1 -R 'own_vector|direct2d.contract'`0/wrapper0,6/6 in128.07sec. Raw `legacy-focused-{build,tests}.log`, capture root `legacy-focused-captures/run-172882670158600`; original failure/ruling qualifications not hidden by these default-suite passes.

## Final frozen combined gates and owning original receipts

Product/test/build/pin source bytes froze after all witness/native-policy/interior edits:289tracked files from `git ls-files src include backends tests CMakeLists.txt cmake third_party/rlottie/UPSTREAM.json`, recorded in ignored `final-source-freeze.csv` SHA256 `2CC5E3330A0F1524DBECC9E70033BC55F137BE275A95A0786715F03DF55D2C76`. Controller committed documentation may change; these289code/build/pin hashes are the protected execution set. No code changes during/after final gates.

Sequential combined final shell initializes VsDevShell again for EACHcache before its build and CTest, in order no-reference/Telegram/explicitWin32preview. Native stdout/stderr Tee raw logs and LASTEXITCODE captured after every native call; nonzero build/test stops wrapper. Core commands percache:
```powershell
cmake --build out/build/windows-msvc-direct2d --config Debug --parallel 4
ctest --test-dir out/build/windows-msvc-direct2d -C Debug --output-on-failure --no-tests=error -j 1
cmake --build out/build/windows-msvc-direct2d-capture --config Debug --parallel 4
ctest --test-dir out/build/windows-msvc-direct2d-capture -C Debug --output-on-failure --no-tests=error -j 1
cmake --build out/build/windows-msvc-win32-preview --config Debug --parallel 4
ctest --test-dir out/build/windows-msvc-win32-preview -C Debug --output-on-failure --no-tests=error -j 1
```
Raw logs `final-full-{no-reference,telegram,preview}-{build,tests}.log`; environment percache `AVEMOTION_TASK_CAPTURE_ROOT=C:/Users/USER/Desktop/AveMotion/out/own-alpha-matte-task3/final-full-{name}-captures`. This is the final combined gate requested by controller; controller will not duplicate it unless review changes code or reveals failure.

| Cache | Native build | CTest | Wrapper | Passed / total | Seconds | C4251 raw warning lines |
| --- | --- | --- | --- | --- | --- | --- |
| no-reference | 0 | 0 | 0 | 47/47 | 8.91 | 0 |
| Telegram | 0 | 0 | 0 | 95/95 | 240.89 | 1 |
| explicit Win32 preview | 0 | 0 | 0 | 96/96 | 240.54 | 2 |

Sequential combined shell exit0; no failed/disabled/skipped tests observed. Every build and CTest initialized in the same shell as stated above. Preview selftest passed; no interactive installed Party UI or hardware observation claimed. Inherited C4251 output remains unsuppressed. Native stdout/stderr and test chronology are in the logs; native/wrapper labels were captured from shell output.

Fresh owned capture roots: Telegram `final-full-telegram-captures/run-173205867260100`, preview `final-full-preview-captures/run-173455413506400`. Each `own-alpha-matte/metrics.tsv` has528 native rows, all native_max0; LastTest.log explicitly reports396 fresh pinnedCPU comparisons, max allowance3. The CPU-pass flag on the remaining132 native-only rows is not an additional CPU execution claim. Each LastTest.log also contains28 fractional capture records, native_diff0/cpu_pass1/skips0 (not33). Existing literal/kernel3 tests are unchanged.

Final frozen original owning commands run sequentially in both reference-enabled caches, same-shell prefix before EACHcache, nonzero build/test stops wrapper:
```powershell
$env:AVEMOTION_TASK_CAPTURE_ROOT='C:/Users/USER/Desktop/AveMotion/out/own-alpha-matte-task3/final-frozen-original-telegram-captures'
cmake --build out/build/windows-msvc-direct2d-capture --config Debug --target avemotion_own_vector_stream_differential_tests avemotion_own_vector_capture_tests --parallel 4
& './out/build/windows-msvc-direct2d-capture/Debug/avemotion_own_vector_stream_differential_tests.exe' --party-original C:/Users/USER/Desktop/AveMotion/out/real-corpus-2026-10-08/assets/noto-1f389.json
& './out/build/windows-msvc-direct2d-capture/Debug/avemotion_own_vector_capture_tests.exe' --party-original C:/Users/USER/Desktop/AveMotion/out/real-corpus-2026-10-08/assets/noto-1f389.json
$env:AVEMOTION_TASK_CAPTURE_ROOT='C:/Users/USER/Desktop/AveMotion/out/own-alpha-matte-task3/final-frozen-original-preview-captures'
cmake --build out/build/windows-msvc-win32-preview --config Debug --target avemotion_own_vector_stream_differential_tests avemotion_own_vector_capture_tests --parallel 4
& './out/build/windows-msvc-win32-preview/Debug/avemotion_own_vector_stream_differential_tests.exe' --party-original C:/Users/USER/Desktop/AveMotion/out/real-corpus-2026-10-08/assets/noto-1f389.json
& './out/build/windows-msvc-win32-preview/Debug/avemotion_own_vector_capture_tests.exe' --party-original C:/Users/USER/Desktop/AveMotion/out/real-corpus-2026-10-08/assets/noto-1f389.json
```
Raw `final-frozen-original-{telegram,preview}-{build,stream_differential,capture}.log`: both builds0, all4 executable native/per-test-wrapper exits0, containing shell0. Each scene executable552samples/max1.52588e-05; each capture20native/20CPU unchanged-policy decisions,310source primitives/skips0, original interiors/corners allowance3, independent general-affine onlydy witness vendor_native_max0/sensitivity_max20. Roots Telegram `run-173717989632700`, preview `run-173740015234700`; both party-metrics.tsv SHA256 `0CBA14FE89FE7E999F849AFF03F9C781CCE637F081F770D032A2CED81CA52317`, exactly20rows/zero native andCPU failures. Six rectangular rows retain max20/7pixels>3; separate final policy decisions preserve that qualification.

Final CPU host artifact for Task4: `final-frozen-original-telegram-captures/run-173717989632700/party-129x129-f0-cpu.bgra`,66564bytes/stride516/premultiplied BGRA8, SHA256 `03593C7BC15E459394601A03214154DEAA8D46763DB6AB683CD0FFD5157F699A`, identical diagnostic3 and preview bytes. Adjacent final provenance SHA256 `24A80C968495D5E7375DAB48375509C83949FBF7F54332320C9038A9159A2ACE` records frozen-source hashes, pin/build/cache/library/executable receipts, exact generation commands and exits. Final capture executable hashes: Telegram `329E0CB8DDF6F31F876CCF6766964702D6627F7B6E8EF7BC809DD0A7A79DCF31`, preview `3A136C57B45E83A7161140C2FF387965B6D1272E4B0595142BD81AE889EB06CC`.

Final289protected source hashes rechecked after all commands: zero mismatches. All three original asset hashes rechecked unchanged. Controller independent523file freeze additionally establishes only the five owned test/helper paths changed since Task2, other518including production/vendor unchanged (`task-3-controller-freeze.csv`, SHA256 `3D497F8583DE74E6720EB3E644CE09E1FE0C475FD727EED0E6693FC14E2C0DC8`). Controller also independently recomputed all40native/CPU policy decisions from the original final20-row TSV; subsequent final frozen TSV hash is identical.

Final selected raw evidence inventory `out/own-alpha-matte-task3/final-raw-evidence-hashes.csv`:114records/zero missing hashes, SHA256 `8D4C164F9DA8691742F9703E07409E80D7E863435D25D277207B0AF8700878AE`. Selection covers logs, metrics, frame0CPU, isolated draw0/literal viewport rasters, provenance and sourcefreeze; it is not a claim to hash every generated whitePPM/BGRA/ordinary export. Earlier incomplete inventory while a capture process held files is superseded, not accepted evidence. All failed provisional/raw roots remain intact.

Final self-review found no production changes, own-field oracle grafts, default strictness changes, public fallback changes or policy/golden edits. Scope is exactly five test/helper files plus public report and this implementer report. Commit subject `test: qualify unchanged Party alpha matte acceptance`; scoped local commit excludes controller STATE/spec/plan and ignored raw/artwork. Independent Task3 review/parent integration remains pending, no push. Limitations: qualified missing ordinary fields, full-native original residual20/7edgepixels, general-affine sufficiency rather than actual intermediate bit identity, incremental Debug/WARP-only evidence and inherited C4251 warnings.
