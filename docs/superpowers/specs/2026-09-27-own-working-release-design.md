# Part26L — own Motion Lab in the working Avelabs EXE

## User intent and delegated approval

On 2026-09-27 the user explicitly requested development directly into Avelabs
`build/Release`, with commits, ordinary pushes and progress comments. This
supersedes Part26K's preserve-the-accepted-EXE restriction for this local working
package. Earlier delegated ordinary technical/spec/plan decisions remain valid;
the controller approves this written design, not a claim of user file review.
Architectural path: this changes the host build/package boundary. Work directly
in main; root-only Git, no history rewriting. Bases engine a816d09/host f452922.

Success: ordinary local static builds produce the own no-rlottie Motion Lab in
the working EXE, visible on Voices without a special command-line flag; a fully
verified package and matching symbols replace the old package with recoverable
backup. Reference lab remains isolated/uninstallable. No engine feature change
is part of L. The user selected controller choice of next example: unchanged
first-party `primitive_geometry` (seven primitives) is the subsequent separate
coverage stage, not a fake claim that L already supports it or a real sticker.

## Selected approach and alternatives

Add explicit `AVELABS_OWN_MOTION_RELEASE` BOOL default OFF, valid only at the
host root for lab ON + own-warp + variant none + static x64 Release /MT. Standard
`build_ui.bat static` selects new `own-static` preset; CMake generic defaults
remain OFF/reference. Preserve `original-static` as explicit legacy control.
Own diagnostic builds follow the same own mode, so they cannot silently remove
the feature. Keep separate intermediate directories, not separate final apps.

Do not simply copy the Part26K EXE: it has no verified matching PDB or complete
own-package notice/provenance record. Do not put reference rlottie in Release:
that would expand licensing/distribution scope and contradict own direction.
No DLL/module loader, alternate player or new UI shell is needed.

## Configure/install and output boundaries

The explicit own-working flag may permit host Runtime install rules only. Keep
`AVEMOTION_ENABLE_INSTALL=OFF`, reference SDK installation prohibition and all
cache isolation. All other lab builds (including own-warp without this flag)
retain existing root/deferred no-install/fresh-tree checks. The flag with lab
OFF, reference renderer, conflicting variant, nonstatic/CRT mismatch or a caller
other than the actual host root fails closed. A configuration with stale
install scripts cannot be repurposed into reference lab mode.

Working mode requires configured install prefix exactly host `build/Release`.
It still builds EXE/PDB in its intermediate tree: failed compilation must not
overwrite the working pair. Runtime install copies the app/metadata/notices;
Symbols component copies matching PDB into `build/symbols`. A staged Runtime
install with an explicit temporary `--prefix` under build/checks is permitted
for pre-promotion validation; it is not another delivered application or SDK.

Presets `own-static` and `own-static-diagnostic` use existing static Qt6.10.0
dependencies, /MT, symbols ON, install prefix build/Release. No dependency
installation. Source path is a required absolute `AVELABS_AVEMOTION_SOURCE_DIR`
cache value, initialized via `build_ui.bat static <absolute-engine-path>` or
explicit configure. The preset must not overwrite it with an absent environment
variable on subsequent builds. No hard-coded developer source path in presets.
The diagnostic tool accepts optional `--engine-source`, otherwise resolves the
configured own-static cache path; fail clearly if unavailable, never fallback
to a lab-OFF build. Both wrappers preserve command failures/exit codes.

## Host behavior and metadata

In own working mode `installMotionLab` attaches the existing page even without
`--motion-lab`, exactly once. Other lab modes retain the opt-in flag. Use one
private compile definition for that distinction; preserve controls, truthful
own-ellipse/WARP badge, thread/timer, idle/visibility policy, tray/Exit/docking,
window settings and DPI. No automatic unsupported asset load in L.

Generated static-build-info preserves current required Qt/MSVC/CRT/plugins
fields and adds explicit `Motion: own-warp`, `AveMotion-Revision: <commit>` and
`Distribution: local-development-only` for working mode. The engine revision
comes from the actual engine checkout; unknown/dirty input must be reported
honestly and cannot pass the final clean promotion. Do not infer binary origin
from a later documentation HEAD. Preserve separate host/engine source evidence.

## Existing notices, not a new licensing decision

This is a local development package, not an external distribution or a selected
public license. Preserve current twenty vcpkg notices. Copy these four existing
engine texts byte-for-byte, do not rewrite their meaning or select a license:

- NOTICE.md -> licenses/avemotion-NOTICE.txt
- LICENSES/LGPL-2.1.txt -> licenses/avemotion-LGPL-2.1.txt
- LICENSES/UNLICENSE-miniz.txt -> licenses/avemotion-UNLICENSE-miniz.txt
- docs/LICENSE_AND_SECURITY.md -> licenses/avemotion-LICENSE-AND-SECURITY.txt

Own package therefore has exactly26 files: EXE, static metadata,24 notices.
The exact original22-file profile remains verifiable. The release verifier
selects own profile only from valid own metadata; missing/unknown/duplicate
mode, malformed/missing revision, missing notices or extra files fails. This
inventory preserves existing notices; it is NOT legal compliance certification.
Current engine source lineage/public-license warnings remain unresolved for
public distribution. Do not add any external sticker assets in L.

## Promotion and verification

TDD covers own-package metadata/layout, configure/refusal/install boundaries,
batch/diagnostic selection and page entry without a flag. Preserve original
tests, including reference no-install. Full deployment/build Python suites;
fresh both-mode Motion Lab QtTests; own working-mode page entry case; independent
task review and whole-stage review. Actual fresh own static app/smoke build,
no rlottie/reference link inputs, Windows-only imports and symbol pairing.
No engine full suite rerun solely for host packaging changes; K frozen engine
inputs must stay identical. No speedup/native DPI/security claim.

Before any change to Release or canonical symbols: root verifies exact resolved
paths and no reparse points; no running working EXE (never close user processes);
creates a new backup directory below build/checks with complete old package and
PDB and verifies hashes. Build candidate in intermediates, test and stage its
Runtime package below build/checks, verify exact layout/notices/imports/metadata
and matching PDB. Then install/promote the verified candidate to build/Release
and matching PDB to build/symbols, recheck readiness/hashes/symbols. Failure after
replacement restores the verified old pair/package, preserving failure evidence.
No recursive deletion of workspace or old evidence. Standard command documents
backup responsibility; first promotion is controller-run, not a new updater.

Preserve all failed attempts, commands, timestamps, both source revisions,
before/after package inventories and backup location. No foreign process kills,
Windows/security/power changes, installs of dependencies, automation or new
publication. Ordinary main pushes only after scoped verification/review and
remote-base check. Final state distinguishes installation from visual/native
acceptance and lists any unverified scenario.

## Self-review and continuation

Explicit opt-in package flag prevents broad weakening of reference guards.
Intermediate outputs and verified backup prevent a build failure from destroying
the working pair. An existing development notice is preserved, not converted
into a redistribution license. Root handles actual promotion after review.
After L, design/implement whole first-party primitive_geometry support against
pinned Telegram oracles in a separate stage; keep its real-sticker limitation.
