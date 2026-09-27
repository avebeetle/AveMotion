# Part26L — own Motion Lab in the working EXE

Completed 2026-09-27. The user's canonical Avelabs package now contains the
existing own ellipse path, statically linked into the same EXE. Voices installs
the Motion Lab page without `--motion-lab`. This is a local development package,
not complete Lottie/SVG support, native visual acceptance or a public release.

## Frozen source and installed artifacts

Host product commits: `699fc0b` (working route) and `56dc299` (partial metadata
rejection). The fresh build used clean host
`56dc299a38fd3d4e98f5a8a7140c7dc18cf399c3` and engine
`e8f21e3d9306d05c06a09bbcf9d3ed3172efe8f4`. L changes no engine product code;
all 923 K product input paths/hashes match. Later report commits are not the
EXE's build identity.

| Artifact | Bytes | SHA-256 |
| --- | ---: | --- |
| Installed `build/Release/AvelabsUI.exe` | 52,569,600 | `93B10E5C93A3182BC59A4333F5444E95F4D4CA32D506AA05E26D1F740C5C1645` |
| Installed `build/symbols/AvelabsUI.pdb` | 156,291,072 | `014ACD8CE9B3AFAB44A80F1ECEEAAC766D606EC958DC08DC64A392767173DFFF` |
| Previous EXE, retained in backup | 52,208,128 | `C92F26EE8FEB4FF03F6DC7F4AFFF4B11FB48142743D1EDCCB4E213AAA81F82C2` |
| Previous PDB, retained in backup | 159,453,184 | `5084E35DBE8D3B519451D0FD1819182C9E0D11FA82CC33BE1BCA73649DB86957` |

All host paths below are relative to
`D:/rvc/c++/DragonianVoice/Avelabs-UI`. Exact own package: 26 files, comprising
EXE, metadata, 20 original notices and four byte-exact engine notice copies.
Metadata records own-warp, engine build revision and local-development-only.
Old complete 22-file package and PDB are hash-verified under
`build/checks/part26l-promotion-001/backup/{Release,symbols}`. Nothing in that
backup was deleted. Promotion finished at `2026-09-27T20:06:29.9193157Z`.

## Verification and review

- Functional TDD for actual package fixtures, real configure/staged Runtime
  rules, wrapper/diagnostic commands and Qt page entry. Task1 deployment95/95,
  build29/29; boundary matrix PASS; reference CTest3/3 (Qt42), own4/4 (Qt53),
  working-entry4/4 (Qt53). Qt counts include lifecycle slots.
- Whole-stage review found two Important issues: partial own metadata could
  downgrade to legacy22; failed evidence capture could prevent first-promotion
  rollback. One combined fix, then exactly one scoped re-review accepted both.
  Fresh root deployment97/97, isolated actual-catch recovery2/2; no failures or
  skips. Recovery fixtures deliberately fail evidence copying and rollback,
  independently of the real canonical files.
- Fresh static app+smoke build with existing Qt6.10, MSVC19.44, x64 Release /MT:
  configure exit0; compile/link `19:54:49.163..19:59:55.275 UTC`, 306.112s, exit0.
  CTest1/1, 0.11s elapsed (smoke0.09s). This is a fresh build tree, not a
  benchmark or a repeat of the unchanged full engine suite.
- Actual graph closures: app12 projects/46 edges; smoke11/44. Two executable
  linker records contain no rlottie/Reference inputs. Eleven actual compiler
  records contain113 /MT commands and no /MD. CMake's two CompilerId Debug
  probes are explicitly excluded/listed, not product build records. No engine
  SDK file-install rule. App/smoke direct+delay import audits pass; they do not
  inspect runtime LoadLibrary. Independently accepted against generated files.
- Candidate26 inventory/notices/metadata passed, old and new EXE/PDB pairs
  matched with installed symchk. Source inventories and old canonical pair
  stayed unchanged throughout compilation/staging. User closed the app; root
  rechecked process identity immediately before promotion, never terminated it.
- After copy, all installed package/PDB hashes match the candidate; installed
  readiness passes and symchk matches GUID `459340b8-9e32-4afb-a728-3d7d43d13af6`,
  age1. `promotion.json` reports `promoted-and-verified`, `guiLaunched:false`.
- Default `own-static` cache initialized with the typed absolute PATH; the
  diagnostic resolver with no explicit source returns this engine tree.
  That configure did not build/install. Routine commands retain the documented
  backup responsibility; they are not an atomic updater.

## Reproducible commands and raw evidence

Use the installed VS2022 x64 developer environment and existing vcpkg tree;
never enable dependency installation. The successful candidate commands were:

```text
cmake --preset own-static -S <host> -B <host>/build/cmake/part26l-own-release-001 -DAVELABS_AVEMOTION_SOURCE_DIR=<engine>
cmake --build <tree> --config Release --target AvelabsUI avemotion_engine_smoke --parallel 4
ctest --test-dir <tree> -C Release --output-on-failure --no-tests=error -j 1 --output-junit <evidence>/ctest.xml
cmake --install <tree> --config Release --component Runtime --prefix <evidence>/candidate
python tools/deployment/release_readiness.py --repo <host> --package <package> --dumpbin <installed-dumpbin> --installed <host>/build/dependencies/installed --output-dir <new-evidence-dir>
python tools/diagnostics/verify_symbols.py --exe <exact-exe> --pdb <exact-pdb> --symchk <installed-symchk>
```

Raw host directories: `build/checks/part26l-task1`, `part26l-final-fix`,
`part26l-promotion-001`. Candidate command JSON files preserve exact expanded
arguments, UTC times and exit codes; promotion, inventories, hashes, logs,
JUnit and source manifests remain local. Engine `out/part26l` retains the
one-shot scripts, artifact acceptance and failed instrument versions;
`out/part26l-design` retains the native-harness boundary. Matching
`.superpowers/sdd/2026-09-27-own-working-release`
retains task reports, full diffs, reviews and RED/GREEN evidence references.

Failures were not hidden: two nested shell-quote launches failed before the
candidate started. Closure instrumentation needed absolute ProjectReference
handling, exclusion of explicitly identified CMake compiler probes, and BOOL
TRUE normalization. All corrected audits inspected the same successful build;
none changed product/tests or triggered a rebuild. The recovery harness had
setup failures and an accidental ~209MB PSObject serialization log, retained;
only the corrected isolated catch failure counts as functional RED. Initial
default-cache configure was untyped, then corrected to PATH for diagnostic
resolution. Existing four UI C4100/C4505 warnings and unused CMake variables
remain. Earlier Task1 ancillary logs were reused as disclosed in its report.

## Limits and delegated rulings

Native GUI, docking/tray interactions and mixed DPI were not tested with this
new EXE: the production harness cannot prove settings isolation. QtTest page
entry/captures are separate evidence. WARP is software rendering with readback,
not proof of hardware GPU acceleration, speedup or zero allocation. No license
selection/public distribution approval, Windows changes, dependency installs,
new automation, vendor/golden edits or public fallback changes occurred.

All six controller rulings and costs are retained in the
[durable ledger](superpowers/ledgers/2026-09-27-own-working-release.md): main/raw
retention; narrow own host package flag; notices/local-only licensing boundary;
installation before wider geometry; routine diagnostic canonical installation
without an updater; and unchanged existing PDB/guard files with coverage in the
new suite. The three unmodified mandatory-list files were explicitly accepted,
not silently omitted. Whole-review declined judgments have dispositions there.

Next: separately design and implement the complete unchanged first-party
`primitive_geometry` fixture (seven shapes) through this same own pipeline and
UI. It is a technical fixture, not a downloaded real Telegram sticker. Read-only
inventory/type-transition notes are prepared; no broader geometry code is
included in the installed L EXE.
