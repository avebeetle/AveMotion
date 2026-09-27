# Real Telegram sticker baseline — 2026-09-28

The user clarified that visible progress must be measured against downloadable
real Telegram animations in Avelabs, not synthetic primitive fixtures alone.
This is a diagnostic checkpoint, not completion of own playback or Part26M.

## Original files and provenance

The [official Telegram import documentation](https://core.telegram.org/import-stickers)
links the [iOS example repository](https://github.com/TelegramMessenger/TelegramStickersImport).
Pinned source commit: `b8951c8a02b245142142d341644d0756cf16278c`.
Its [example controller](https://github.com/TelegramMessenger/TelegramStickersImport/blob/b8951c8a02b245142142d341644d0756cf16278c/Example/Source/ExampleController.swift)
loads these as animated stickers. Each is512x512,60fps,180 frames.

| File | Embedded identity | Encoded / JSON bytes | SHA-256 (unchanged TGS) |
| --- | --- | --- | --- |
| [a-1.tgs](https://raw.githubusercontent.com/TelegramMessenger/TelegramStickersImport/b8951c8a02b245142142d341644d0756cf16278c/Example/Source/a-1.tgs) | `_DUCK8_SAD_OUT` | 8877 /132102 | 68A296BA26965440CA75D5C66BFC07385079250B8E7D4C104D365F11E8497E2A |
| [a-2.tgs](https://raw.githubusercontent.com/TelegramMessenger/TelegramStickersImport/b8951c8a02b245142142d341644d0756cf16278c/Example/Source/a-2.tgs) | `_DUCK11_X3_OUT` | 5397 /52883 | 2CDBF4D03DE421B6E0BC73871F4FC7ABB685EF3B3B30B581E8AB75C71501B15A |
| [a-3.tgs](https://raw.githubusercontent.com/TelegramMessenger/TelegramStickersImport/b8951c8a02b245142142d341644d0756cf16278c/Example/Source/a-3.tgs) | `_DUCK4_THINK_OUT` | 4635 /60315 | 7CD55718288EB1B1D1EDFD0E774D9038E076A7F5AADF9835A92CF942948A4BC1 |

Files stay in ignored `out/real-stickers-2026-09-28/assets`; no source sticker,
decoded JSON or rendered image is committed. Repository SDK MIT text is not
treated as independent clearance to redistribute artwork. No licensing change,
bot/account interaction, import/upload or mass archive download occurred.

## Observed results

| Check | Result | What it proves |
| --- | --- | --- |
| Existing strict production TGS decoder |3/3 decoded | Original transport accepted, no rewrite of sticker data |
| Existing Telegram-backed corpus lab |3/3 loaded and sampled | Reference-supported files; all3 reported fallback, nativeDirect2DReady0 |
| Ordinary reference characterizer |15/15 rendered | Frames0,45,90,135,179 per file; nonempty pixels and distinct frame hashes |
| Current own reader/factory/preparation scratch probe |0/3 real files accepted | All stop at JsonRead/ResourceLimit; no own render or UI success claim |
| Same own probe, unchanged basic + primitive fixtures |2/2 preparation Ready | Positive controls only, not completion of multi-item parity |

Own reader is still bounded at4096 values. Captured parsed node counts before
its breadth-first resource rejection are23816,9736,11114 respectively. This is
the first observed blocker, not the complete unsupported-feature list. Literal
diagnostic paths and JSON byte counts are in `own-probe.tsv`.

The reference feature inventory shows precompositions, shape paths, solid
strokes, trim paths and spatial position animation. Those inventory labels
describe the existing reference-backed analysis, NOT support in the new own
factory. For a-3 specifically:1 precomposition,32 shape paths,15 solid strokes,
7 trim paths,33 tracks and304 segments. Current own grammar remains far narrower.
Merely increasing the reader limit would not produce correct own playback.

Selected next acceptance asset: **a-3.tgs, Duck think**. It has one root layer and
one precomposition asset; that is a useful bounded starting target, not a promise
it is trivial or already supported. Next implementation plan must inventory its
actual properties and target its unchanged playback in the existing Avelabs
worker/Player/canvas, with reference comparisons. Synthetic tests remain lower-level
regressions; their completion is not the user-visible success criterion.

## Reproducible local evidence

All paths below are under `out/real-stickers-2026-09-28`:

- `sources.md`: pinned URLs, source identity and rights caveat.
- `own-probe.cpp`, `build-own-probe.cmd`, `build-own-probe.log`, `own-probe.tsv`:
  diagnostic uses the same TGS/readOwnJson/buildOwnNativeEllipseModel/preparation
  functions called by OwnMotionRenderer, not a parser imitation. Compiled MSVC
  Debug /MDd against current none-build libraries; exit0 means probe finished,
  not that rejected assets passed. Not the installed static host executable.
- `reference-corpus-001/`: manifest/features/issues/benchmark rows and summary;
  stdout in `reference-corpus-001.log`, invocation recorded below. Incidental
  timings are not a controlled performance experiment or speedup evidence.
- `decode-for-reference.ps1`: bounded decoding to unchanged JSON for the existing
  JSON-only characterizer. `reference-frames-001/manifest.tsv` identifies pinned
  Telegram rlottie67f103bc8b625f2a4a9e94f1d8c7bd84c5a08d1d and all15 frame hashes.
- Characterizer PPM snapshots are original outputs; `encode-reference-png.ps1`
  losslessly encodes RGB snapshots to PNG for viewing. PPM cannot be opened by
  the image tool directly. Duck-think frame90 was visually inspected. The image
  is reference CPU output on the characterizer's black background, not our UI.
- `probe-binary-hashes.json`: exact probe/reference executable and none-library
  hashes. Engine HEADcc589b0 plus frozen uncommitted M Task2 changes; no claim
  that these are clean-commit final artifacts. Installed L host was not changed.

Commands from the engine root (installed MSVC environment for scratch compile):

```text
out/real-stickers-2026-09-28/build-own-probe.cmd
out/real-stickers-2026-09-28/own-probe.exe out/real-stickers-2026-09-28/assets/a-1.tgs out/real-stickers-2026-09-28/assets/a-2.tgs out/real-stickers-2026-09-28/assets/a-3.tgs tests/fixtures/primitive_geometry.json tests/fixtures/telegram_sticker_basic.json
out/build/windows-msvc-telegram-debug/avemotion_corpus_lab.exe --input out/real-stickers-2026-09-28/assets --output out/real-stickers-2026-09-28/reference-corpus-001 --samples 5 --warmup-samples 0 --load-repeats 1 --cpu-repeats 1 --render-size 128 --include-names --max-files 3 --max-total-mib 1
out/build/windows-msvc-telegram-debug/avemotion_characterize.exe --output out/real-stickers-2026-09-28/reference-frames-001 --size 256 out/real-stickers-2026-09-28/decoded/a-1.json out/real-stickers-2026-09-28/decoded/a-2.json out/real-stickers-2026-09-28/decoded/a-3.json
```

These are15 reference phase samples, not exhaustive180-frame parity. No hardware
GPU benchmark, native/DPI/tray acceptance, full Lottie compatibility, third-party
redistribution approval or own real-sticker playback is claimed.

## M checkpoint and revised priority

Part26M Task1 is already reviewed/pushed. Task2 focused model/resources/stream
tests pass, but first multi-item differential sample fails
`scene.drawItems[1].canonicalPaint.sourceKey`;3 legacy tests pass. Capture/full
gates, independent task review, host integration and installation are unfinished.
Writer frozen with22 modified/new source/test files preserved. Partial report:
`.superpowers/sdd/2026-09-27-own-primitives-profile/task-2-report.md`.

Decision: stop advancing the synthetic-only M acceptance chain while defining
real-sticker missing-feature coverage. Cost: M remains an uncommitted partial
checkpoint, not a shipped feature. Do not undo it or push it as verified product.
Keep current canonical L26-file EXE/PDB and backups. User-visible target is the
actual Duck animation in the same Avelabs UI, then additional real stickers.
