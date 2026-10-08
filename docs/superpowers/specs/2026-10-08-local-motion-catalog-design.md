# Local motion catalog design

Status: controller-approved under the user's explicit delegation of reversible
specification, planning and execution decisions. No new licensing, dependency,
backend, public distribution or Windows-setting decision is delegated here.

## Outcome

The same Avelabs Voices page offers a small, preconfigured local animation list.
An app-local catalog supplies titles and recorded provenance without opening a
picker on every session. Only the selected asset is prepared by the existing
worker. The own engine, software WARP presentation and 1/4/16 controls remain.
An absent catalog leaves the present picker/last-file behavior intact.

Host base c6419d265c6ae6bc4d3570cf9dc85313dc50822c; engine base
c2cd58a4280b824b5ac0ee1fa03dfafe239ef75f. Engine production code is not changed.
The previous native acceptance gap remains open; it is not silently completed.

## Alternatives and decision

Keep only last-file restoration: smallest, but no discoverable list or configured
first selection. Scan arbitrary directories: simple-looking, but loses explicit
provenance and bounded membership. Chosen approach: one bounded manifest and a
selector on the existing page. No network loader, watcher, persistent frame cache,
second worker or generic asset database.

## Catalog boundary

Production default is QStandardPaths::AppLocalDataLocation plus
`/motion-catalog/manifest.json`. An explicit page constructor accepting a manifest
path provides test isolation; an explicitly empty path disables catalog loading.
The existing constructor delegates to the production default. Tests use only
temporary injected paths; no developer catalog is read by a test.

The existing MainWindow route test retains installMotionLab unchanged. Because
that production entry constructs the default page, this one test uses
QStandardPaths test mode and a unique per-run test application identity derived
from a QTemporaryDir basename before constructing MainWindow. Assert that its
default manifest path is isolated and absent. Keep INI settings redirected by the
existing test helper; every directly constructed page uses a temporary manifest
or the explicitly empty path. No production entry API or app identity changes.

Read metadata once on first effective visibility. Maximum manifest size is
65536 bytes, maximum entries 64. Require an object with integer `version: 1`,
`defaultId` naming one entry and nonempty `assets`. Each entry has unique `id`
(ASCII letters/digits/_/-, 1..64), `title` (1..128 characters), `file` (1..256),
`sha256` (exactly 64 hex digits), `source` (1..2048), `license` (1..512), and
`useScope: "local-evaluation-only"`. Unknown JSON keys may be ignored; duplicate
asset IDs or invalid entries invalidate the whole manifest. Limits are enforced
before publication. Titles/provenance are plain text, never executable links/HTML.
License/source strings are recorded declarations, not authenticity or permission.

Asset `file` is slash-separated relative to the manifest's directory. Reject
backslashes, absolute/drive/UNC/device paths, colons, NUL, empty or dot/dot-dot
components, trailing dot/space and Windows reserved device component names.
Only `.json`/`.tgs` extensions are admitted, case-insensitively. No asset bytes
or canonical-file scan are needed to populate the list.

The selected request carries its root and raw 32-byte expected SHA256 to the
existing worker. Resolve root/selected canonical paths with a separator boundary
and Windows case-insensitive containment; reject escaping links/junctions and
missing/non-file entries there. The worker reads at most the existing 2 MiB,
hashes that exact QByteArray and prepares the same bytes, without reopening.
Mismatch is an ordinary load error before preparation/install. A digest without
root, root without valid digest, or non-32-byte digest is rejected. Manual
requests have empty root/digest and retain the current behavior.

This is a trusted local-development catalog, not a hostile-filesystem sandbox.
Canonical path checks can race a concurrent reparse swap; SHA256 ensures
consistency with the declared bytes, not a signed authority. In-root hardlinks
are not prohibited. No native handle sandbox or security-sensitive system change.

### Windows final path clarification

The retained Qt 6.10 regression shows that QFileInfo canonicalFilePath leaves an
escaping directory junction unresolved on this machine. On Windows the resolver
therefore obtains the final root and selected-file names through metadata handles
using CreateFileW and GetFinalPathNameByHandleW. Normalize DOS paths, reject
unresolvable paths, close both handles deterministically, and compare the final
names with the existing separator and case-insensitive containment rule. Other
platforms keep the Qt canonical-path implementation.

These handles are only for path resolution. The existing worker still reads one
bounded QByteArray, verifies its declared digest, and prepares that same buffer.
This does not remove the documented concurrent reparse race or add a hostile
filesystem sandbox. No dependency, permissions, rendering or fallback change.
The controller approves this bounded clarification under the existing delegated
authority; the escaping-junction functional RED must pass without weakening it.

## Requests and persistence

Add optional digest/root to the existing file load request. File, digest and root
coalesce as one unit, including clearing both on a new manual request. Transport,
visibility, count and size intents must not detach that binding. Keep existing
generation filtering and cancellation; never persist an obsolete loaded result.

Selection is lazy: populate metadata on effective visibility; hidden selections
may replace a pending selection but may not prepare an asset until visible.
Automatic first/default/restored selection autoplays through the existing flag.
Explicit selector selection, like picker import, loads paused. Pause/Stop/Seek
and slider press cancel pending autoplay, including while hidden/in flight.
No automatic retry on repeated hide/show, hash failure or unsupported content.

Success chooses one restoration mode. Catalog success writes
`MotionLab/lastCatalogId` and removes `MotionLab/lastLocalAsset`; manual success
writes the accepted lastLocalAsset and removes lastCatalogId. Failure changes
neither. This deliberate change makes the last successful user choice win without
turning a catalog file into an unverified manual restore. Clear pending catalog
identity before manual/new requests and on accepted failure/success.

For preexisting/conflicting settings, a nonempty lastLocalAsset keeps the old
manual restoration priority. Otherwise restore a nonempty lastCatalogId; missing
ID/catalog reports one ordinary error, without default fallback. With neither
saved choice, a valid catalog default is selected automatically. Invalid/missing
catalog does not prevent manual import. Expose a catalog-specific error label
so catalog metadata failure does not overwrite an active worker error/session.
No automatic rendering fallback to reference, including on unknown IDs.

The selector contains a non-loading placeholder plus catalog entries. Programmatic
population/selection is signal-blocked. It must not imply success before accepted
loaded; on failed replacement the error is visible and the saved choice remains.
Choosing the placeholder cancels a not-yet-submitted hidden catalog selection and
its pending autoplay. It submits no worker request, does not unload an accepted
session and changes no saved restoration keys. It does not separately invalidate
an already submitted worker result; existing request generations still apply.
Retain canvas, route badge, count/transport/diagnostics and overall page bounds.

## Local seed and rights

Only existing unchanged a-2/a-3 are seeded for this developer machine, outside
build/Release and Git. Their TelegramStickersImport b8951c8 source revision,
source paths, exact hashes and unresolved artwork-redistribution status appear
in manifest provenance. MIT SDK licensing is not asserted as artwork permission.
Publish a newly created verified asset subdirectory first and manifest last;
never overwrite a preexisting different manifest or artwork. Preserve partial
attempts. No new Internet acquisition or unlicensed archive is required.

The package remains exactly 26 files, notices intact, PDB separate. No sticker
bytes or local catalog file enter package/Git. First-use availability is proved
only for a machine with a valid local catalog, not a newly installed empty PC.

## Acceptance

TDD: functional RED before corresponding production logic, retained raw evidence.
Metadata bounds/invalid fields/traversal/device names and duplicate IDs; real
worker digest match/mismatch, root escape and mailbox tuple replacement; page
lazy default/list/restored ID, success-only mode switching, preserved manual
priority, invalid catalog/manual recovery, supersession and transport cancellation.
An actual same-drive relative manual test closes the previous deferred coverage
gap only if it asserts its input is relative; no false cross-drive coverage claim.

Independent review after each task and once for the entire stage. Fresh own /MD
full suite, focused original a-2/a-3 page/worker checks, reference compatibility
where changed common request interfaces apply; fresh actual own Release /MT build,
smoke, source binding, imports/no-reference link, notices/package/PDB gates.
Guarded promotion to canonical build/Release only when both running Avelabs
instances have exited. Never kill user processes. Native capture/input limitations
and external-gate skips remain explicit; no benchmark, speedup or GPU claim.
