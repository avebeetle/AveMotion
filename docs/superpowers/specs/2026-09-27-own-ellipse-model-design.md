# Part26G — own ellipse model and resources

Baseline: accepted Part26F c00f686450c1fd725b39e543badd42cdf23df20c;
tested880020d, full none34/34, Telegram80/80 and preview74/74. Do not repeat F.
Controller designs and approves under the user's explicit delegation of written
spec/plan/SDD decisions; this is not a claim that the user reviewed unseen files.

## Intent and success

The destination is an own Lottie/TGS engine statically linked into the Avelabs-UI
EXE. Telegram remains a compatibility reference, not a permanent production
dependency. This architectural slice constructs the already-admitted narrow
ellipse model and canonical resources without reference parsing, frame scans,
Asset allocation or vendor headers. It must work in AVEMOTION_RLOTTIE_VARIANT=none.
Existing production Runtime and experimental reference-CPU Motion Lab stay intact.

Success means an immutable first-party authored graph works with the existing
PropertyEvaluator, and a separately sealed resource preparation produces complete
render rows/local static resources usable by the existing model application code.
It does NOT yet authorize the current reference-certified stream to consume this
model. Own stream identity/frame/pixel proof and static host integration follow.
No general Lottie feature expansion, UI redesign or new renderer is part of G.

## Alternatives and chosen boundaries

Choose a distinct own authored-model producer in Runtime, followed by an own
resource preparer in Rendering. Rendering already depends on Runtime/Model;
keeping primitive-path use there avoids a Runtime-to-Rendering dependency cycle.
Both are existing static targets: no new DLL, plugin mechanism or target family.

Rejected: fabricate an Asset/handle or loosen NativeEllipseCertificate::matchesAsset.
That would confuse proof from reference scanning with proof from direct own
construction. Also rejected: replace the general Runtime loader now, or duplicate
the complete model/evaluator in a second engine. Reuse existing first-party data,
numeric interpretation, binder, evaluator, path generator and hash algorithms.

## Global constraints

- Direct main, small scoped commits and ordinary push after verification/review; no force-push, rebase or history rewrite. One product writer; preserve raw/SDD evidence.
- No vendor, dependency, license/notice, fixture or golden changes; no dependency installation or Windows/settings/security/automation actions.
- No UI writes/builds/GUI or UI/out recreation; preserve accepted Avelabs712d454 and Release EXE C92F26EE8FEB4FF03F6DC7F4AFFF4B11FB48142743D1EDCCB4E213AAA81F82C2.
- No Runtime.cpp loader/identity changes, Player threading, Direct2D ownership, ANGLE/backend coverage or public fallback changes. All new interfaces are private/not installed; reference-linked install prohibition stays.
- NativeEllipseCertificate, NativeEllipseScanAudit, legacy admission/input contracts and all existing tests remain unchanged. NativeEllipseStream changes only by extracting its identical first-party path materializer, not its authorization, emission or lifetime policy.
- OwnJsonReader and the F grammar/adapters are unchanged; do not copy vendor parsing, add a mutable own-document builder, reserialize, or fall back to a reference route.
- No full Lottie, own playback/GPU, universal scalar parity, TSan, zero-allocation or measured-speedup claim. Samsung is configure/provenance-only in this stage.

## Shared numeric interpretation and binding

Extract the existing canonical decimal-to-double-to-float conversion and complete
input numeric validation from NativeEllipseBinding.cpp into a private compiled
NativeEllipseNumeric.hpp/.cpp. Keep its exact order, canonical representation,
finite/range checks, signed-zero normalization and nonzero-underflow rejection.
Return an optional complete NativeEllipseNumericValues value; no partial output.
The old binder calls this one implementation and retains all binding error codes
and precedence. Compile binder and numeric implementation in Runtime for every
variant; no reference includes or new public/install exposure.

Fields: float frameRate; MotionVec2Value translation,size,start,end,outgoing,
incoming; MotionColorValue color; bool animated; uint32 firstFrame,lastFrame.
For static input, unused endpoint/control/frame fields remain value-initialized.
Do not relax conversion because F admission accepts arbitrarily tiny decimals:
exact grammar acceptance and representability by this float model are separate.

## Own authored model

Private Runtime factory buildOwnNativeEllipseModel(const OwnJsonDocument&)
first calls F own admission. It constructs no model on rejection; the exact
admission code/path is retained. Next interpret numerics; unsupported conversion
is its own result, not an altered JSON/admission failure. Build the graph below,
run bindNativeEllipseModel and prepare a PropertyEvaluator before publication.
Unexpected construction/binding/evaluator disagreement returns a typed internal
model error without publication, never a reference fallback.

Publish only a shared_ptr<const OwnNativeEllipseModel> through a private
constructor/friend factory. It owns exactJson, input, immutable model, interpreted
values and the checked binding. Input/doc destruction cannot invalidate anything.
No caller supplies arbitrary model rows or a source hash to this factory.

Authored conventions are producer-selected, not copied oracle row assignments:

| Data | Convention |
| --- | --- |
| Schema, metadata |schema2, revision1; dimensions/endFrame from input; frameRate double of interpreted float; debugName present root nm including empty, absent gives empty|
| Identity |assetHandle invalid (not a Runtime Asset); sourceAssetHash raw-byte FNV-1a of exactJson, no string-length prefix|
| Composition |row0/id0, rootNode0, name root, firstFrame0, endFrame input.endFrame, metadata equal model|
| Source nodes |IDs0..4 = root/layer/group/ellipse/fill; parents invalid,0,1,2,2; composition0; child edges[1,2,3,4]; ranges root0/1,layer1/1,group2/2,leaves0/0|
| Property edges |[0..7]; root0/0,layer0/2,group2/2,ellipse4/2,fill6/2|
| Properties0..7 |layer matrix,layer opacity,group matrix,group opacity,ellipse position,ellipse size,fill color,fill opacity; owner IDs1,1,2,2,3,3,4,4; semanticIndex0|
| Typed values |scalars[100,100,100]; matrices[layer translation,identity]; color[interpreted RGBA]; static vec2[position,size], animated vec2[start,end,size]|
| Static value indexes |opacities scalar0/1/2, transforms matrix0/1, color0; position vec2 0 if static; size vec2 1 static or2 animated|
| Animation |only property4 animated, no static reference, track0; one segment0, range[0,endFrame-1], start vec2 0/end1, outgoing/incoming controls; Linear only controls(0,0)/(1,1), otherwise CubicBezier; no spatial interpolation|

All present flags/IDs/ranges must be explicit and valid. Static properties have
PropertyFlagStatic and invalid track; unused tracks/segments/value tables empty.
Names are the binder's exact effective defaults: root=root; layer nonempty nm or
layer:<ind>; group nonempty nm or group; ellipse/fill nonempty nm or source-node.
All name hashes use Fnv1a64::appendString (length-prefixed). Layer alone has Shape
kind, authored layer ID, active half-open interval; others have None/-1/0 defaults.
Root/layer/group/ellipse are timeline-dependent and not authoredStatic only for
animated position; fill always static. Preserve every binder default for enabled,
hidden, parenting, references, time stretch, matte/mask/blend, solid/gradient,
stroke/trim/repeater/path direction. No field inherits undocumented vendor state.

The authored model has no render rows/resources/clips yet. Statistics report
directParsedModel=true,1 composition,5 source nodes,8 properties, static8/animated0
or7/1; tracks/segments0/0 or1/1; scalar3,vec2 2 or3,color1,matrix2,shape/gradient0.
Here directParsedModel means authored data, not Telegram provenance.

Own parsedModelFingerprint is FNV appendString("AveMotion.OwnEllipse.Authored.v1")
then appendString(exactJson). It is a versioned source-derived diagnostic/cache
fingerprint, not equal to Telegram's table fingerprint, not cryptographic identity
and not a proof token. Exact whitespace changes it intentionally. No hash-only
authorization. Existing evaluator workspaces are prepared separately per model;
tests pin rejection of a workspace prepared for a different source fingerprint.
For authored-only models, other render-derived fingerprints remain default0.

## Own resource preparation

Private Rendering factory prepareOwnNativeEllipseAsset(shared_ptr<const
OwnNativeEllipseModel>) accepts only that sealed authored owner. Null returns
InvalidModel. It retains the owner, copies its small immutable authored model into
a private mutable construction buffer, fills render tables/resources, refreshes
derived statistics/fingerprints, then publishes a const full model in a separately
sealed OwnNativeEllipsePreparedAsset. No mutable model alias survives publication.
Retaining both authored and full tables costs bounded duplicate small graph data;
no memory improvement is claimed. This object is not runtime::Asset and holds no
invented handle, reference certificate, scan counts or Runtime registration.

Render rows: root Layer0/no parent,shape Layer1/parent0, childLayerIds[1],
layerNodeIds[0], root children0/1,nodes0/0; shape children0/0,nodes0/1; no masks/
matte/layer dependencies. Node0/draw0 links layer1/geometry0/paint0,drawOrder0,
dependency StaticDependencyTransform; drawOrder[0]. Default Clip0 named default,
[0,endFrame),Loop. Declared/observed counts2/1/1/1,mask0,clip1.

Own render names/key paths are literal display labels: root __; shape is the
effective authored layer name above, including every decoded byte. They are not
parsed hierarchical selectors. No extra rejection/escaping is introduced for
slashes, periods, brackets or Unicode. Future selector APIs need their own design;
do not claim arbitrary-name key-path equality with Telegram. Existing nm/zero-byte
admission restrictions remain. Render nameHash uses appendString(label).

Static position geometry0 is AssetStatic: first-party generateEllipsePath using
interpreted local position/size and Clockwise; materialize without a transform,
with identical verb/point/bounds/hash operations extracted from NativeEllipseStream.
Layer translation and viewport must not be baked into this local geometry.
Animated geometry0 is InstanceEvaluated,hash0,no static value; this preparation
does not promise every animated float position has a nondegenerate emitted path.
Invalid/nonfinite static primitive materialization returns ResourceConstructionFailed
and publishes nothing. Neither admission nor numeric policy is weakened to hide it.

Paint0 is always AssetStatic, sourceKey1, Solid, RGB byte truncation uint8(255*float)
and alpha255, disabled default stroke/no dashes and default inactive gradient/image.
Static geometry sourceKey1 and Winding. Share the existing Model resource hash
implementations through private helpers, not copied algorithms. Expose a private
refreshAssetModelDerivedData wrapper around existing statistics/fingerprint logic;
legacy scan call sites keep behavior. Own full model retains its own parsed
fingerprint and uses the existing topology/resource/full derived fingerprints.

The path materializer extraction is mechanical into Rendering for both old stream
and own preparation. No general path framework or new renderer. A prepared own
resource model must pass the binder again; existing applyAssetModel must alias
static geometry/paint from this model, with animated geometry remaining evaluated.

## Verification and remaining proof boundary

TDD functional compiling stubs before model/prepared publication. Independent
literal tests cover all graph/property/value/default fields, active intervals,
static/animated endpoints/controls, names, numeric exact-to-float limits, lifetime,
failure atomicity and separate concurrent preparations. Existing legacy tests and
goldens remain unchanged. Direct none tests prove no runtime reference dependency.

Geometry witness position0,size2 uses explicit six verbs/13 literal points and
independently computed constant hashes; paint witnesses fractional RGB0.5/0.1/
0.999 give127/25/254 and alpha255. Pin resource/application ownership, sourceKey,
stats, typed-row bounds, zero/nonfinite failure, source hash and own fingerprint.
No production helper generates its own expected value. Shared binder validation
and old/new equality supplement, never replace literal expectations.

Telegram-only tests compare stated semantic authored/evaluated properties and
canonical resources for common admitted/representable inputs, not raw ID vectors,
source/parsed/full fingerprints, Asset handles, revision counters or arbitrary
name key-path spelling. Assert both sides independently on representative cases
and make comparator self-tests detect changed values/roles/defaults/resources.
No oracle-assisted own construction, no mismatch whitelist, no new fixtures.
Own-specific lexical/Unicode policies and tiny-number conversion rejection remain
explicit. Reference certificate may appear only in the comparison test executable.

Final gates: independent task/whole-stage reviews; full MSVC none/Telegram and
Win32-preview including capture/WARP/device recreation; Samsung configure only;
vendor/TGS16 integrity; actual includes/link/source-owner/private-install boundary
and accepted UI hash preservation. No new performance claim from test timings.

After G, separately design own stream identity and adapt the existing first-party
emitter without weakening old certificate checks. Prove frames, access order,
visibility, lifetimes, pixels and bounded scheduling before enabling a native path
in an isolated static Avelabs-UI build. G is not that host/playback completion.

## Self-review and delegated approval

Root read the binder, own document/input, model, evaluator ownership checks, path
generator and Model hash/summary code, plus the two source-only design inventories.
The resource preparer stays in Rendering to preserve the target DAG. Own invalid
Runtime handle is explicit and cannot pass the reference certificate. No general
loader, hidden scan or claimed full-frame guarantee is introduced. Numeric policy,
name labels, hash/provenance distinction and temporary duplicate graph storage are
explicit decisions. Controller approves this written spec under delegation;
next writing-plans/self-review/SDD preflight are mandatory before product changes.
