# Task 1 report — shared compiled ellipse admission core

Base: `13d260310a22a449937b4fc27ee8fd44edd27b85`.

## Implementation

- Added the private Runtime `NativeEllipseAdmissionCore.hpp/.cpp` borrowed-table
  contract and the single compiled exact-decimal/schema/materialization implementation.
  It validates its internal table (root, reachability, immediate-child links, scalar
  constraints, key placement, lexical number tokens), clears a supplied output before
  work, and leaves audit-only calls descriptor-free.
- Kept `NativeEllipseAdmission.cpp` as the Telegram-only compatibility front end:
  unchanged byte/NUL gate, pinned DOM flags, resource/duplicate BFS and raw SAX
  alignment occur before it projects DOM/event-owned views to the core. The core
  therefore factors numeric normalization only after full alignment.
- Added unconditional none/core CMake test wiring, Telegram-only compatibility wiring,
  literal fixture/projection helpers, and core/compatibility tests. Core tests cover
  literal complete output, non-contiguous immediate siblings, remapped-row links,
  audit-only, the Ruling 6 Fill-before-ellipse witness (`UnsupportedField` at
  `/layers/0/shapes/0/it/0/c`), rejection publication clearing, descriptor ownership,
  and invalid-internal-table errors. Compatibility tests pin the specified legacy
  admissions, paths (including NUL bytes), descriptors, names, numeric edge cases,
  resources, trailing syntax, and static/animated variants.

The extraction review found `ExactDecimal`, `Auditor`, and `rootObject` only in
`src/runtime/NativeEllipseAdmissionCore.cpp`; the legacy frontend contains only
frontend capture/alignment/projection. The core header includes only the existing
private `NativeEllipseInput.hpp` plus standard headers. No vendor, reader, Runtime
route, fixture/golden, install-policy, UI, dependency, settings, or automation file
was changed.

## TDD and retained historical evidence

The retained first implementation evidence in `out/part26f/task1/` is from
2026-09-24, not this resume:

- `01-legacy-baseline.log`: old Telegram admission/input baseline, 2/2 passing.
- `02-none-configure.log` and `03-none-stub-build.log`: successful none configure and
  stub build before functional testing.
- `04-core-functional-red.log`: real functional RED: valid projected fixture did not
  emit an accepted descriptor (`core emits accepted descriptor`); this was not a
  missing-symbol/build RED.
- `12-core-first-green.log`: core 1/1 passing after extraction; `14-telegram-first-green.log`:
  core plus admission/input/compat 4/4 passing. `15`–`18` retain topology/expanded
  compatibility GREEN evidence.
- `20-invalid-token-red.log`: real correction RED for invalid lexical table number
  token; `22-invalid-token-green.log` records its 1/1 GREEN after validation was added.
- `24-none-full-ctest.log`: historical none full suite 33/33; `26-telegram-focused-regression.log`:
  historical Telegram focused regression 8/8. They are expressly historical, not
  presented as fresh resume evidence.

## Fresh resume evidence (2026-09-27)

All commands used the actual VS 2022 BuildTools `VsDevCmd.bat` with `-arch=x64
-host_arch=x64`; complete non-overwriting output and exit-status files are in
`out/part26f/task1/resume-20260927/`.

| Evidence | Command/result |
| --- | --- |
| `01`–`03` | `cmake --preset windows-msvc-direct2d`; core target build; focused none core CTest — all successful, core 1/1. |
| `04`–`05` | `cmake --preset windows-msvc-telegram-debug`; build core/compat/legacy admission/input/binding/certificate/stream targets — successful. |
| `07` | Telegram focused CTest for core, compat, legacy admission/input, binding, certificate — 6/6 passing. |
| `08` | Telegram native ellipse stream and lifecycle CTest — 2/2 passing. |
| `09`–`10` | full none build then full CTest — build successful; 33/33 passing. |

`06-telegram-focused-ctest.log` is retained as a harness warning: Windows `cmd`
interpreted the unquoted `|` in the regex and reported `admission_compat` as a command;
no CTest ran. `07` repeats the intended test selection with correct Windows quoting
and passes. The initially assumed Community `VsDevCmd` path was absent; `vswhere`
resolved the installed BuildTools path used by every recorded fresh command.

## Self-review and scope

- `git diff --check` passed (only Git's pre-existing LF-to-CRLF informational warnings
  for tracked files were emitted).
- Source/CMake review confirms the core target is outside vendor guards and compatibility
  wiring remains inside the existing Telegram guard.
- The existing dirty docs (`docs/superpowers/STATE.md`, plan and ledger) are controller
  work and were not staged or edited by this task. The only scoped paths to stage are
  `CMakeLists.txt`, the two Runtime core/frontend sources, and the three Task 1 test/helper
  files plus this report.

## Commit and quiescence

Product commit: `4c9237d refactor: share first-party ellipse admission core`.
No build/test process was running before the fresh checks; final process state is
checked after the report evidence commit and recorded in the handoff message.
