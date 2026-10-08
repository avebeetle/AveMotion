# Real animation corpus: first diverse sample and input-boundary correction

This is local development evidence, not new product asset distribution or a
general Lottie compatibility claim. The requested direction is automatic asset
availability in Avelabs UI, general feature coverage driven by a diverse corpus,
then actual Release 1/4/16 measurements. There is no 30–40-animation product cap.

## Local acquisition and rights

Three unchanged Animated Noto Emoji Lottie JSON files were acquired from the
official Google Fonts CDN on 8 October 2026 (Moscow local date). Google's
[Animated Noto documentation](https://googlefonts.github.io/noto-emoji-files/)
explicitly identifies CC BY 4.0 for these animations. The samples are local
evaluation inputs under ignored `out/real-corpus-2026-10-08/assets/`; they are
not added to Git or bundled into Avelabs. Any later distribution must separately
preserve required attribution, source/license links and modification notices.
Asset permissions do not settle AveMotion's public code-license decision.

| Input / official download | Original bytes | Parsed nodes | Local SHA-256 |
|---|---:|---:|---|
| [Fire, U+1F525](https://fonts.gstatic.com/s/e/notoemoji/latest/1f525/lottie.json) | 29358 | 5167 | `96199D8E8FEA7D90196B95B9A9E56B13AF43E3817EFE037BD9AA1E5215579838` |
| [Party Popper, U+1F389](https://fonts.gstatic.com/s/e/notoemoji/latest/1f389/lottie.json) | 67875 | 12939 | `EAB1FD7070BFFD2965C12A80D2D91CFF76683AF65019F142ACF9A553EE877B5C` |
| [Turtle, U+1F422](https://fonts.gstatic.com/s/e/notoemoji/latest/1f422/lottie.json) | 55887 | 9593 | `A441CF9A8363C30BA82DF8209B9D1B8F0A4A772FCEAC0B17BC181F09D966EF7D` |

The CDN `latest` URLs are mutable. The table pins the acquired bytes by hash;
fetching those URLs later does not establish the same input. These are real
published vector animations, not evidence that arbitrary Telegram sticker packs
are supported. Existing real TelegramStickersImport a-2/a-3 remain unchanged local
evaluation inputs: their artwork redistribution permission is still unresolved.
See [the Duck X3 report](PART26O_DUCK_X3_ENGINE_REPORT.md).

## Reproduced input-boundary failure

Baseline engine source `cd340f5be6a3569f86bd9fe311b35f8934ba8722`, host source
`1e286e4e6e1232c92a2b4ed3da0cf3a498d098a2`. The installed frozen O build is
engine `9576920e168a956fba14f573df1bc380187d4a0e` / host
`2b9982ff9312df80b4f7cdc9887d72860c93adff`; later documentation HEADs are not its
build revisions.

The host's reader permits 65536 JSON nodes. A document containing `el` or `rc`
selects the bounded primitive compiler, whose internal value table permits 4096.
The own adapter previously projected the wider document without checking that
compiler limit. Unchanged Fire reached an internal invariant and threw
`invalid internal ellipse root`; the separate diagnostic showed a CRT abort
dialog. The installed O engine smoke also exited `0xC0000409` on Fire. Party
Popper and Turtle instead returned ordinary rejection at `/markers`.

Correction: both own input adapters now reject more than 4096 values as
`NativeEllipseAdmissionCode::ResourceLimit` at `/`, with no input owner, before
projection/evaluation. One private constant is shared with the unchanged core
invariant. Empty input remains `InvalidJson`; exactly 4096 values still reach
ordinary grammar checks. No cap increase, grammar extension, fallback retry,
asset-name branch, renderer change or broad production exception catch.

The independently authored regression uses the real reader with a 65536-node
budget and explicitly checks 4096/4097-node documents through both adapters.
MSVC functional RED observed both previous exceptions. Targeted GREEN passes
2/2 existing test executables; independent task review approved the correction.
Fresh full MSVC builds and CTest pass: no-reference 47/47 (9.92 s), Telegram
100/100 (124.00 s), explicit `windows-msvc-win32-preview` 96/96 (143.98 s).
These are Debug/WARP functional gates, not Release playback performance. The
inherited Telegram C4251 header warning remains; no golden changes were made.
The exact command wrapper is
`out/real-corpus-2026-10-08/boundary-fix/run-full-gates.cmd`.

Relinking the diagnostic to the corrected no-reference libraries and feeding
all five unchanged inputs returns normally: Fire now returns prepare
`InvalidModel` at `/` with `own primitive admission rejected`, Party/Turtle
remain rejected at `/markers`, a-2/a-3 prepare successfully. Adapter-level tests
verify the more specific ResourceLimit code; the existing preparation facade
does not expose it. Normal unsupported rejection is not playback acceptance.

An extra attempted a-2 property-only differential command failed its
`unique semantic role` precondition: that unchanged legacy comparator uses
`precomp` and node kinds without sibling/instance ordinals. It is not weakened
or reported passing. The established Part26O instance-aware full-scene/capture
route is used for X3; a-3 keeps its four ordinary model/stream regressions.
The initial wrapper also passed a relative path to a tool requiring absolute
`--tgs`; that command error was corrected without changing any assertion.
Both failures are disclosed in the retained scratch execution note.

The established original-input regression run exits 0: a-2 full scene 2960
samples, a-3 four model/stream tools (180-frame property oracle and 440 scene
samples), and 68/68 exact WARP capture pairs per asset under the unchanged CPU
comparison policy. No pixel skips were reported. a-2 final path/matrix maximum
is `1.52588e-05`, stroke maximum `2.47955e-05`; a-3 final path maximum is
`3.05176e-05`. Large raw local-space differences remain reported (a-2 up to
`33.6390705109`, a-3 `0.0001220703125`); the pre-existing independent paint-space
correspondence predicts them rather than altering reference geometry/tolerances.
See `run-duck-regressions.ps1`, `a2-full-scene.log`, the a-3 tool logs and each
asset's capture log in the evidence directory. These are engine tests, not a
native UI control or installed-EXE acceptance.

At the admission checkpoint the correction was not promoted. Installed
`D:/rvc/c++/DragonianVoice/Avelabs-UI/build/Release/AvelabsUI.exe` still hashes to
`94DBDE53F2EF59991783C8134C6BC6747719C44AF2D5BA00D4F6A0B3B210B6BA`.
That frozen O image contained the old admission path, not the fix. The subsequent
static promotion described below supersedes this installed-image status.
Raw RED/GREEN, build/test logs and XML remain under
`out/real-corpus-2026-10-08/boundary-fix/`; the execution ledger and review records
are in `.superpowers/sdd/primitive-admission-boundary-plan/`.

## Subsequent static delivery and automatic restoration

On 8 October the canonical host package was built from clean engine
`e589335d37ac89c61752f42427a761bbd891490e` and clean host product
`08ea99d920b790f74bcd9d4f5eb6a36b5c08b2d7`, including this admission correction.
The host now remembers the last accepted local file and restores/plays it once
on effective Voices visibility through the existing worker/Player. Manual
load stays paused; transport intent supersedes delayed autoplay. This is not
a catalog or first-launch bundled artwork, and no paths/asset names are compiled
in. The first successful local choice remains manual.

Promotion was verified `2026-10-08T09:55:56.3456489Z`; installed EXE SHA256
`5C75BCDAAE89179BE613E764BDC46302B1A15058E06FAEBF4E53E38336C15182`, matched PDB
`01298EE0D7A48DA46CE270D998CDBD26B998063C2CE5DC748630E393364D232B`.
Fresh actual static /MT build/smoke, original a-2 smoke, clean source binding,
119actual /MT target commands, no-reference app link and staged/installed
package/import/symbol checks pass. All26package files and notices are verified;
previousO/PDB backup and raw evidence remain in host
`build/checks/2026-10-08-auto-motion-001`.

Native load/control/restart and Fire rejection in that EXE remain pending.
Computer Use capture failed twice and observed-button input then failed
`coordinate input geometry is unavailable`; no blind input or repeated loop.
The app is left open. QtTest4/4, focused fixture/Duck31/0/0 and original Duck
page3/0/0 prove host mechanics but not native acceptance; five explicit external
gate skips in the full /MD suite are disclosed. No Noto playback or speedup claim.
Final independent stage review approved integration with0Critical/0Important
and one disclosed test-coverage Minor: cross-drive fixtures did not exercise
manual relative expansion. The coverage statement is corrected and that test
is deferred; unsafe saved-relative automatic restoration is tested separately.
Supplemental actual /MT original a-3 smoke exits0; Fire normal exit2 at the smoke's
combined preparation/admission check proves neither exact diagnostic nor native
UI acceptance. No production fix or repeated rebuild is required for this Minor.

## Telegram ideas applicable to automatic availability

Source comparison is pinned to tdesktop
`d346b42a1d30ef60dc989b6e5191bb8e571f6bd5` and its lib_lottie submodule
`7d00b5048aff8dd93c0a9721abf50006d20682be`; no upstream code is copied here.

- [The panel creates eligible loaded animations lazily while painting](https://github.com/telegramdesktop/tdesktop/blob/d346b42a1d30ef60dc989b6e5191bb8e571f6bd5/Telegram/SourceFiles/chat_helpers/stickers_list_widget.cpp#L2210), with [asynchronous preparation](https://github.com/desktop-app/lib_lottie/blob/7d00b5048aff8dd93c0a9721abf50006d20682be/lottie/lottie_animation.cpp#L141).
- [Saved frames/thumbnails appear before animation readiness](https://github.com/telegramdesktop/tdesktop/blob/d346b42a1d30ef60dc989b6e5191bb8e571f6bd5/Telegram/SourceFiles/chat_helpers/stickers_list_widget.cpp#L2263); [invisible rows pause and distant heavy state is released](https://github.com/telegramdesktop/tdesktop/blob/d346b42a1d30ef60dc989b6e5191bb8e571f6bd5/Telegram/SourceFiles/chat_helpers/stickers_list_widget.cpp#L1915).
- [Frame-cache entries distinguish size/replacement requests](https://github.com/telegramdesktop/tdesktop/blob/d346b42a1d30ef60dc989b6e5191bb8e571f6bd5/Telegram/SourceFiles/chat_helpers/stickers_lottie.cpp#L42). This does not establish that every ordinary instance shares one parsed model.

Applicable minimal next design: a rights-known local catalog with stable IDs,
paths, hashes and provenance; automatic default/last valid selection through the
existing worker; immediate preview and lazy selected/visible preparation. Keep
the picker as a diagnostic import path. Reuse current immutable prepared asset
sharing, visibility freeze and generation filtering. Add bounded cross-asset
memory or persistent raster caching only if measurements justify it. Do not
import Telegram's account/network infrastructure or redesign the shell.

The supplied 8 October video shows Telegram's panel, not Avelabs native Duck X3
control acceptance. Video alone does not identify vector TGS versus video
stickers, network/cache conditions or engine performance. Personal video frames
remain local and are not committed.

## Measurement boundaries and next gates

The installed own route is Direct2D software WARP with QImage readback, not a
hardware-GPU optimization claim. Current 1/4/16 controls repeat one prepared
asset. Sequential different-asset runs are not heterogeneous concurrency.
Existing diagnostics and the host process observer can measure bounded warm
same-asset runs. Cold load, first *painted* frame, separate readback cost and
displayed-frame drops require small hooks at existing boundaries; last render
elapsed time is not thread CPU time. The historical dynamic `/MD` controller
harness is not proof of current static `/MT` installed-UI performance.

Next order: retain native current-EXE Duck controls/restart as pending; last-local
automatic restoration is installed, but a catalog is still a separate step. Use
exact corpus failures to
choose general missing features (do not merely strip fields from originals);
then record uncontended Release 1/4/16 phases at fixed actual size/DPI/visibility.
Record errors, CPU/memory/counter deltas and raw output before optimizing.
No performance improvement or new Noto playback acceptance is claimed here.
