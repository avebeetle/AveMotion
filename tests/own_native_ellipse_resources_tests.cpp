#include "AssetModelBuilder.hpp"
#include "NativeEllipseAdmissionTestData.hpp"
#include "NativeEllipsePathMaterializer.hpp"
#include "OwnJsonReader.hpp"
#include "OwnNativeEllipseModel.hpp"
#include "OwnNativeEllipseModelTestData.hpp"
#include "OwnNativeEllipsePreparedAsset.hpp"
#include "PrimitivePathGenerator.hpp"
#include "avemotion/formats/Tgs.hpp"

#include <array>
#include <bit>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>

namespace {
using namespace avemotion;
using namespace avemotion::model;
void require(bool value, std::string_view text) {
    if (!value) {
        throw std::runtime_error(std::string{text});
    }
}

// Test-owned FNV-1a encoding: no product hashing helper constructs expectations.
struct Fnv {
    std::uint64_t v = 14695981039346656037ULL;

    void b(std::uint8_t x) { v = (v ^ x) * 1099511628211ULL; }
    void u32(std::uint32_t x) {
        for (unsigned i = 0; i != 4; ++i) {
            b(static_cast<std::uint8_t>(x >> (8 * i)));
        }
    }
    void u64(std::uint64_t x) {
        for (unsigned i = 0; i != 8; ++i) {
            b(static_cast<std::uint8_t>(x >> (8 * i)));
        }
    }
    void f64(double x) {
        if (x == 0) {
            x = 0;
        }
        u64(std::bit_cast<std::uint64_t>(x));
    }
    void text(std::string_view x) {
        u64(x.size());
        for (char c : x) {
            b(static_cast<std::uint8_t>(c));
        }
    }
};

std::uint64_t raw(std::string_view s) {
    Fnv h;
    for (char c : s) {
        h.b(static_cast<std::uint8_t>(c));
    }
    return h.v;
}

std::uint64_t nameHash(std::string_view s) {
    Fnv h;
    h.text(s);
    return h.v;
}

std::uint64_t parsedHash(std::string_view s) {
    Fnv h;
    h.text("AveMotion.OwnEllipse.Authored.v1");
    h.text(s);
    return h.v;
}

std::uint64_t topologyHash(std::string_view layer, bool animated) {
    Fnv h; h.u32(2); h.u64(2); h.u64(1); h.u64(1); h.u64(1); h.u64(1);
    h.b(1); h.u32(0); h.u32(kInvalidModelId); h.u32(0); h.u32(1); h.u32(0); h.u32(0); h.u32(0); h.u32(0); h.u64(nameHash("__")); h.u32(0); h.b(0);
    h.b(1); h.u32(1); h.u32(0); h.u32(0); h.u32(0); h.u32(0); h.u32(1); h.u32(0); h.u32(0); h.u64(nameHash(layer)); h.u32(0); h.b(0);
    h.u32(1); h.u32(0); h.b(1); h.u32(0); h.u32(0); h.u32(1); h.u32(0); h.u32(0); h.u32(0); h.u32(animated ? 3U : 1U); h.u32(0); h.b(1); h.u32(0); h.f64(0); h.f64(61); h.b(1); return h.v;
}
std::uint64_t resourceHash(std::uint64_t geometry, std::uint64_t paint, bool animated) { Fnv h; h.u64(1); h.b(1); h.u32(0); h.b(animated?1:2); h.u64(animated?0:geometry); h.u64(1); h.b(1); h.u32(0); h.b(2); h.u64(paint); return h.v; }
std::uint64_t fullHash(std::string_view s, std::uint64_t topology, std::uint64_t resources) { Fnv h; h.u32(2); h.u64(raw(s)); h.u64(512); h.u64(512); h.f64(60); h.u64(61); h.u64(topology); h.u64(resources); h.u64(parsedHash(s)); return h.v; }

std::string repl(std::string s, std::string_view a, std::string_view b) {
    return test::replaceEllipseOnce(std::move(s), a, b);
}

std::string staticSource() {
    auto s = test::staticEllipseFixture(test::ellipseFixture());
    s = repl(std::move(s), "[-32768,32768]", "[0,0]");
    s = repl(std::move(s), "[120, 120]", "[2, 2]");
    return repl(std::move(s), "[0.08, 0.72, 0.95, 1]", "[0.5, 0.1, 0.999, 1]");
}

runtime::detail::NativeEllipseInput literalInput() {
    using runtime::detail::NativeEllipseInput;
    using runtime::detail::NativeEllipseStaticPosition;
    const auto decimal = [](bool negative, const char* significand, bool powerNegative = false,
                            const char* power = "0") {
        return runtime::detail::NativeEllipseDecimal{
            negative, significand, {powerNegative, power}};
    };

    NativeEllipseInput input;
    input.width = 512;
    input.height = 512;
    input.endFrame = 61;
    input.frameRate = decimal(false, "6", false, "1");
    input.layerId = 1;
    input.layerInFrame = 0;
    input.layerOutFrame = 61;
    input.layerTranslation = {decimal(false, "256"), decimal(false, "256")};
    input.size = {decimal(false, "2"), decimal(false, "2")};
    input.position = NativeEllipseStaticPosition{{decimal(false, "0"), decimal(false, "0")}};
    input.fillColor = {decimal(false, "5", true, "1"), decimal(false, "1", true, "1"),
        decimal(false, "999", true, "3"), decimal(false, "1")};
    input.version = "5.7.4";
    input.name = "AveMotion Telegram sticker profile fixture";
    input.layerName = "Moving Circle";
    input.groupName = "Circle Group";
    input.ellipseName = "Animated Ellipse";
    input.fillName = "Fill";
    input.transformName = "Transform";
    return input;
}

runtime::detail::NativeEllipseNumericValues literalValues() {
    return {60, {256, 256}, {2, 2}, {0, 0}, {}, {}, {}, {.5F, .1F, .999F, 1}, false, 0, 0};
}

std::shared_ptr<const runtime::detail::OwnNativeEllipseModel> owner(std::string_view s) {
    const auto json = formats::detail::readOwnJson(s);
    require(static_cast<bool>(json), "source parsed");
    const auto result = runtime::detail::buildOwnNativeEllipseModel(*json.document);
    require(static_cast<bool>(result), "authored model");
    return result.prepared;
}

std::shared_ptr<const render::detail::OwnNativeEllipsePreparedAsset> prep(std::string_view s) {
    const auto result = render::detail::prepareOwnNativeEllipseAsset(owner(s));
    require(static_cast<bool>(result), "own resources prepared");
    return result.prepared;
}

void requireCompleteRenderRows(const MotionAssetModel& m) {
    require(m.layers.size() == 2 && m.nodes.size() == 1 && m.geometries.size() == 1
            && m.paints.size() == 1 && m.clips.size() == 1
            && m.drawOrder == std::vector<NodeId>{makeId<NodeId>(0)}
            && m.childLayerIds == std::vector<LayerId>{makeId<LayerId>(1)}
            && m.layerNodeIds == std::vector<NodeId>{makeId<NodeId>(0)},
        "all render table counts/order");
    const auto& root = m.layers[0];
    const auto& shape = m.layers[1];
    const auto& node = m.nodes[0];
    const auto& geometry = m.geometries[0];
    const auto& paint = m.paints[0];
    const auto& clip = m.clips[0];
    require(root.present && root.id==makeId<LayerId>(0) && !root.parent.valid() && root.children.first==0 && root.children.count==1 && root.nodes.first==0 && root.nodes.count==0 && root.masks.first==0 && root.masks.count==0 && root.debugName=="__" && root.nameHash==nameHash("__") && root.dependencyBits==StaticDependencyNone && root.matte==runtime::MatteMode::None,"root all fields");
    require(shape.present && shape.id==makeId<LayerId>(1) && shape.parent==makeId<LayerId>(0) && shape.children.first==0 && shape.children.count==0 && shape.nodes.first==0 && shape.nodes.count==1 && shape.masks.first==0 && shape.masks.count==0 && shape.debugName=="Moving Circle" && shape.nameHash==nameHash("Moving Circle") && shape.dependencyBits==StaticDependencyNone && shape.matte==runtime::MatteMode::None,"shape all fields");
    require(node.present && node.id==makeId<NodeId>(0) && node.drawItem==makeId<DrawItemId>(0) && node.layer==makeId<LayerId>(1) && node.geometry==makeId<GeometryId>(0) && node.paint==makeId<PaintId>(0) && node.drawOrder==0 && node.dependencyBits==StaticDependencyTransform,"node all fields");
    require(geometry.present && geometry.id==makeId<GeometryId>(0) && geometry.resourceClass==ResourceClass::AssetStatic && geometry.contentHash==0x6862cc6516f16586ULL && geometry.staticValue && paint.present && paint.id==makeId<PaintId>(0) && paint.resourceClass==ResourceClass::AssetStatic && paint.contentHash==0x8485243b1edad747ULL && paint.staticValue,"resource record fields");
    require(clip.present && clip.id==makeId<ClipId>(0) && clip.debugName=="default" && clip.firstFrame==0 && clip.endFrame==61 && clip.defaultLoop==ClipLoopHint::Loop,"clip all fields");
}

void requireCompleteStatistics(const MotionAssetModelStatistics& s) {
    require(s.declaredLayerCount==2 && s.declaredNodeCount==1 && s.declaredGeometryCount==1 && s.declaredPaintCount==1 && s.observedLayerCount==2 && s.observedNodeCount==1 && s.observedGeometryCount==1 && s.observedPaintCount==1 && s.assetStaticGeometryCount==1 && s.assetStaticPaintCount==1 && s.maskCount==0 && s.clipCount==1,"render statistics");
    require(s.directParsedModel && s.compositionCount==1 && s.sourceNodeCount==5 && s.propertyCount==8 && s.staticPropertyCount==8 && s.animatedPropertyCount==0 && s.trackCount==0 && s.segmentCount==0 && s.scalarValueCount==3 && s.vec2ValueCount==2 && s.colorValueCount==1 && s.matrixValueCount==2 && s.shapeValueCount==0 && s.gradientValueCount==0,"post-refresh authored statistics");
}

void literalStaticRowsAndHashes() {
    const auto source=staticSource(); auto authored=owner(source); auto result=render::detail::prepareOwnNativeEllipseAsset(authored); require(result && result.code==render::detail::OwnNativeEllipsePrepareCode::Ready,"static ready"); const auto& m=*result.prepared->model;
    test::assertOwnAuthoredModel(*result.prepared->authored, literalInput(), literalValues(), source);
    require(result.prepared->authored==authored && m.revision==1 && !m.assetHandle.valid(),"retained sealed owner");
    requireCompleteRenderRows(m);
    requireCompleteStatistics(m.statistics);
    require(m.layers.size()==2 && m.nodes.size()==1 && m.geometries.size()==1 && m.paints.size()==1 && m.clips.size()==1 && m.childLayerIds==std::vector<LayerId>{makeId<LayerId>(1)} && m.layerNodeIds==std::vector<NodeId>{makeId<NodeId>(0)},"all render rows");
    require(m.layers[0].present && !m.layers[0].parent.valid() && m.layers[0].children.first==0 && m.layers[0].children.count==1 && m.layers[0].nodes.count==0 && m.layers[0].masks.count==0 && m.layers[0].debugName=="__" && m.layers[0].nameHash==nameHash("__") && m.layers[0].dependencyBits==0 && m.layers[0].matte==runtime::MatteMode::None,"root fields");
    require(m.layers[1].present && m.layers[1].parent==makeId<LayerId>(0) && m.layers[1].nodes.first==0 && m.layers[1].nodes.count==1 && m.layers[1].debugName=="Moving Circle" && m.layers[1].nameHash==nameHash("Moving Circle"),"shape fields");
    require(m.nodes[0].present && m.nodes[0].drawItem==makeId<DrawItemId>(0) && m.nodes[0].layer==makeId<LayerId>(1) && m.nodes[0].geometry==makeId<GeometryId>(0) && m.nodes[0].paint==makeId<PaintId>(0) && m.nodes[0].dependencyBits==StaticDependencyTransform,"static dependency");
    require(m.clips[0].present && m.clips[0].debugName=="default" && m.clips[0].firstFrame==0 && m.clips[0].endFrame==61 && m.clips[0].defaultLoop==ClipLoopHint::Loop,"clip fields");
    const auto& g=*m.geometries[0].staticValue; constexpr float k=0.5522847498F; const std::array<runtime::PathVerb,6> verbs{runtime::PathVerb::MoveTo,runtime::PathVerb::CubicTo,runtime::PathVerb::CubicTo,runtime::PathVerb::CubicTo,runtime::PathVerb::CubicTo,runtime::PathVerb::Close}; const std::array<runtime::Vec2,13> pts{{{0,-1},{k,-1},{1,-k},{1,0},{1,k},{k,1},{0,1},{-k,1},{-1,k},{-1,0},{-1,-k},{-k,-1},{0,-1}}};
    bool literalPoints=g.path.points.size()==pts.size(); for(std::size_t i=0;literalPoints && i<pts.size();++i) literalPoints=g.path.points[i].x==pts[i].x && g.path.points[i].y==pts[i].y;
    require(g.sourceKey==1 && g.fillRule==runtime::FillRule::Winding && g.path.verbs==std::vector<runtime::PathVerb>(verbs.begin(),verbs.end()) && literalPoints && g.path.controlBounds.valid && g.path.controlBounds.left==-1 && g.path.controlBounds.top==-1 && g.path.controlBounds.right==1 && g.path.controlBounds.bottom==1 && g.path.hash==0x501de312ce7d322aULL && g.contentHash==0x6862cc6516f16586ULL,"literal path/hash");
    const auto& p=*m.paints[0].staticValue; bool imageZero=true; for(float v:p.paint.image.matrix) imageZero=imageZero && v==0; require(p.sourceKey==1 && p.contentHash==0x8485243b1edad747ULL && !p.stroke.enabled && p.stroke.width==0 && p.stroke.miterLimit==0 && p.stroke.cap==runtime::LineCap::Flat && p.stroke.join==runtime::LineJoin::Miter && p.stroke.dashArray.empty() && p.paint.kind==runtime::PaintKind::Solid && p.paint.solid.r==127 && p.paint.solid.g==25 && p.paint.solid.b==254 && p.paint.solid.a==255 && p.paint.gradient.kind==runtime::GradientKind::Linear && p.paint.gradient.start.x==0 && p.paint.gradient.start.y==0 && p.paint.gradient.end.x==0 && p.paint.gradient.end.y==0 && p.paint.gradient.center.x==0 && p.paint.gradient.center.y==0 && p.paint.gradient.focal.x==0 && p.paint.gradient.focal.y==0 && p.paint.gradient.centerRadius==0 && p.paint.gradient.focalRadius==0 && p.paint.gradient.stops.empty() && !p.paint.image.present && p.paint.image.width==0 && p.paint.image.height==0 && imageZero,"paint/default/hash");
    require(m.statistics.declaredLayerCount==2 && m.statistics.declaredNodeCount==1 && m.statistics.declaredGeometryCount==1 && m.statistics.declaredPaintCount==1 && m.statistics.observedLayerCount==2 && m.statistics.observedNodeCount==1 && m.statistics.observedGeometryCount==1 && m.statistics.observedPaintCount==1 && m.statistics.assetStaticGeometryCount==1 && m.statistics.assetStaticPaintCount==1 && m.statistics.maskCount==0 && m.statistics.clipCount==1,"summary fields");
    const auto topology=topologyHash("Moving Circle",false); const auto resources=resourceHash(0x6862cc6516f16586ULL,0x8485243b1edad747ULL,false);
    require(m.sourceAssetHash==raw(source) && m.parsedModelFingerprint==parsedHash(source) && m.topologyFingerprint==topology && m.resourceFingerprint==resources && m.fingerprint==fullHash(source,topology,resources),"independent source topology resource full fingerprints");
}

void animatedFailuresAndApplication() {
    const auto source=test::ellipseFixture(); auto a=prep(source); const auto& m=*a->model; require(m.geometries[0].resourceClass==ResourceClass::InstanceEvaluated && !m.geometries[0].staticValue && m.geometries[0].contentHash==0 && m.nodes[0].dependencyBits==(StaticDependencyTransform|StaticDependencyGeometry) && m.paints[0].resourceClass==ResourceClass::AssetStatic,"animated canonical status/dependency");
    const auto animatedTopology=topologyHash("Moving Circle",true); const auto animatedResources=resourceHash(0,m.paints[0].contentHash,true); require(m.topologyFingerprint==animatedTopology && m.resourceFingerprint==animatedResources && m.fingerprint==fullHash(source,animatedTopology,animatedResources),"animated encoded fingerprints");
    auto active=repl(source,"      \"ip\": 0,","      \"ip\": 10,"); active=repl(std::move(active),"      \"op\": 61,","      \"op\": 20,"); require(prep(active)->model->clips[0].firstFrame==0 && prep(active)->model->clips[0].endFrame==61,"active10to20 preserves default clip");
    auto boundary=test::staticEllipseFixture(source); auto boundaryPrepared=prep(boundary); const auto& boundaryPath=boundaryPrepared->model->geometries[0].staticValue->path; require(boundaryPath.controlBounds.left==-32828 && boundaryPath.controlBounds.right==-32708 && boundaryPath.controlBounds.top==32708 && boundaryPath.controlBounds.bottom==32828,"static local geometry ignores layer translation");
    auto absent=source; absent=repl(std::move(absent),",\n  \"nm\": \"AveMotion Telegram sticker profile fixture\"",""); absent=repl(std::move(absent),",\n      \"nm\": \"Moving Circle\"",""); require(prep(absent)->model->layers[1].debugName=="layer:1","absent name label default");
    auto emptyName=repl(source,"\"Moving Circle\"","\"\""); require(prep(emptyName)->model->layers[1].debugName=="layer:1","empty name label default");
    auto literalName=repl(source,"\"Moving Circle\"","\"a.b/[x]~ \\u2603\""); require(prep(literalName)->model->layers[1].debugName=="a.b/[x]~ \xE2\x98\x83","UTF8 delimiter label remains literal");
    auto collapsed=test::staticEllipseFixture(test::ellipseFixture()); collapsed=repl(std::move(collapsed),"[120, 120]","[1e-45, 120]"); auto o=owner(collapsed); auto collapsePrimitive=render::detail::generateEllipsePath(o->values.start,o->values.size,SourcePathDirection::Clockwise); runtime::EvaluatedPath collapsePath;
    require(collapsePrimitive.valid && collapsePrimitive.verbCount==0 && render::detail::materializeNativeEllipsePath(collapsePrimitive,nullptr,collapsePath) && collapsePath.verbs.empty(),"exact [-32768,32768] skinny primitive is valid empty cause"); auto stable=prep(staticSource()); const auto stableModel=stable->model; const auto stableFingerprint=stableModel->fingerprint; const auto stableGeometry=stableModel->geometries[0].staticValue->path.hash; const auto stablePaint=stableModel->paints[0].staticValue->contentHash; const auto bad=render::detail::prepareOwnNativeEllipseAsset(o); require(!bad && bad.code==render::detail::OwnNativeEllipsePrepareCode::ResourceConstructionFailed && !bad.prepared && stable->model==stableModel && stable->model->fingerprint==stableFingerprint && stable->model->geometries[0].staticValue->path.hash==stableGeometry && stable->model->paints[0].staticValue->contentHash==stablePaint,"failed preparation leaves prior prepared object stable");
    runtime::EvaluatedPath out; auto normal=render::detail::generateEllipsePath({0,0},{2,2},SourcePathDirection::Clockwise); auto invalid=normal; invalid.valid=false; require(!render::detail::materializeNativeEllipsePath(invalid,nullptr,out),"invalid primitive"); runtime::AffineTransform inf{1,0,0,1,std::numeric_limits<float>::infinity(),0}; require(!render::detail::materializeNativeEllipsePath(normal,&inf,out),"nonfinite transform"); require(!render::detail::prepareOwnNativeEllipseAsset(nullptr),"null invalid");
    auto s=prep(staticSource()); runtime::EvaluatedScene scene; scene.sourceAssetHash=raw(staticSource()); scene.drawItems.resize(1); auto& item=scene.drawItems[0]; item.modelNode=makeId<NodeId>(0); item.modelGeometry=makeId<GeometryId>(0); item.modelPaint=makeId<PaintId>(0); item.localGeometryAvailable=item.localPaintAvailable=true; require(detail::applyAssetModel(s->model,scene) && item.geometryOrigin==runtime::EvaluatedValueOrigin::AssetStatic && item.paintOrigin==runtime::EvaluatedValueOrigin::AssetStatic && item.canonicalGeometry.get()==std::addressof(*s->model->geometries[0].staticValue) && item.canonicalPaint.get()==std::addressof(*s->model->paints[0].staticValue),"alias application"); auto firstGeometry=item.canonicalGeometry.get(); require(detail::applyAssetModel(s->model,scene) && item.canonicalGeometry.get()==firstGeometry,"repeated application stable static pointer");
    runtime::EvaluatedScene wrong; wrong.sourceAssetHash=raw(staticSource())^1ULL; wrong.drawItems.resize(1); auto& wrongItem=wrong.drawItems[0]; wrongItem.modelNode=makeId<NodeId>(0); wrongItem.modelGeometry=makeId<GeometryId>(0); wrongItem.modelPaint=makeId<PaintId>(0); wrongItem.localGeometryAvailable=wrongItem.localPaintAvailable=true; require(!wrong.assetModel && !wrong.assetModelApplied && !wrongItem.canonicalGeometry && !wrongItem.canonicalPaint && wrongItem.geometryOrigin==runtime::EvaluatedValueOrigin::InstanceEvaluated && wrongItem.paintOrigin==runtime::EvaluatedValueOrigin::InstanceEvaluated,"fresh wrong-hash scene is unapplied"); require(!detail::applyAssetModel(s->model,wrong) && !wrong.assetModel && !wrong.assetModelApplied && !wrongItem.canonicalGeometry && !wrongItem.canonicalPaint && wrongItem.geometryOrigin==runtime::EvaluatedValueOrigin::InstanceEvaluated && wrongItem.paintOrigin==runtime::EvaluatedValueOrigin::InstanceEvaluated,"wrong source fails without application state"); auto ga=item.canonicalGeometry; auto pa=item.canonicalPaint; s.reset(); scene.assetModel.reset(); require(ga->path.hash==0x501de312ce7d322aULL && pa->paint.solid.a==255,"source authored prepared owners may drop while aliases live");
    runtime::EvaluatedScene animatedScene; animatedScene.sourceAssetHash=raw(source); animatedScene.drawItems.resize(1); auto& animatedItem=animatedScene.drawItems[0]; animatedItem.modelNode=makeId<NodeId>(0); animatedItem.modelGeometry=makeId<GeometryId>(0); animatedItem.modelPaint=makeId<PaintId>(0); animatedItem.localGeometryAvailable=animatedItem.localPaintAvailable=true; require(detail::applyAssetModel(a->model,animatedScene) && animatedItem.geometryOrigin==runtime::EvaluatedValueOrigin::InstanceEvaluated && !animatedItem.canonicalGeometry && animatedItem.paintOrigin==runtime::EvaluatedValueOrigin::AssetStatic && animatedItem.canonicalPaint.get()==std::addressof(*a->model->paints[0].staticValue),"animated application exact paint alias");
    auto tgs=formats::decodeTgsFile(std::filesystem::path{AVEMOTION_TGS_DIR}/"telegram_sticker_basic.tgs"); require(static_cast<bool>(tgs) && prep(tgs.json),"TGS own pipeline none");
    auto changed=repl(staticSource(),"[0.5, 0.1, 0.999, 1]","[1, 0, 1, 1]"); changed=repl(std::move(changed),"[256, 256, 0]","[11, 22, 0]"); auto other=prep(changed); auto original=prep(staticSource()); require(other->model!=original->model && other->authored->exactJson==changed && original->authored->exactJson==staticSource() && other->model->sourceAssetHash!=original->model->sourceAssetHash && other->model->matrixValues[0].dx==11 && other->model->matrixValues[0].dy==22 && std::addressof(*other->model->paints[0].staticValue)!=std::addressof(*original->model->paints[0].staticValue) && other->model->paints[0].staticValue->paint.solid.r==255 && other->model->paints[0].staticValue->paint.solid.g==0 && other->model->paints[0].staticValue->paint.solid.b==255,"distinct translation paint source bytes and immutable tables");
}

void rgbEndpointPaintsUseEveryChannelEndpoint() {
    const std::array<std::pair<std::string_view, runtime::Color8>, 8> cases{{
        {"[0, 0, 0, 1]", {0,0,0,255}}, {"[0, 0, 1, 1]", {0,0,255,255}},
        {"[0, 1, 0, 1]", {0,255,0,255}}, {"[0, 1, 1, 1]", {0,255,255,255}},
        {"[1, 0, 0, 1]", {255,0,0,255}}, {"[1, 0, 1, 1]", {255,0,255,255}},
        {"[1, 1, 0, 1]", {255,255,0,255}}, {"[1, 1, 1, 1]", {255,255,255,255}},
    }};
    for (const auto& [encoded, expected] : cases) {
        const auto source=repl(staticSource(),"[0.5, 0.1, 0.999, 1]",encoded);
        const auto asset=prep(source); const auto& actual=asset->model->paints[0].staticValue->paint.solid;
        require(actual.r==expected.r && actual.g==expected.g && actual.b==expected.b && actual.a==expected.a,"RGB endpoint paint bytes");
    }
}
}
int main(){try{literalStaticRowsAndHashes();animatedFailuresAndApplication();rgbEndpointPaintsUseEveryChannelEndpoint();}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
