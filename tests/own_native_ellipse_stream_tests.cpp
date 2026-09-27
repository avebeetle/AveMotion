#include "NativeEllipseAdmissionTestData.hpp"
#include "NativeEllipseEvaluationHelpers.hpp"
#include "OwnNativeEllipseModelTestData.hpp"
#include "OwnNativeEllipseStream.hpp"
#include "OwnNativeEllipseStreamCounters.hpp"
#include "OwnNativeEllipseStreamTestData.hpp"
#include "avemotion/formats/Tgs.hpp"
#include "avemotion/render/RenderPlanner.hpp"
#include "avemotion/runtime/RecordingBackend.hpp"

#include <array>
#include <atomic>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>
#include <type_traits>
#include <vector>

namespace {
using namespace avemotion;
using render::detail::OwnNativeEllipseStream;
using render::detail::OwnNativeEllipseCreateCode;
using render::detail::OwnNativeEllipseFrameCode;

template<class Value>
void require(const Value& value, const char* message) {
    if (!static_cast<bool>(value)) throw std::runtime_error(message);
}

std::string replace(std::string source, std::string_view from, std::string_view to) {
    return test::replaceEllipseOnce(std::move(source), from, to);
}

std::string animatedSource() { return test::ellipseFixture(); }

std::string staticSource() {
    auto source = test::staticEllipseFixture(animatedSource());
    source = replace(std::move(source), "[-32768,32768]", "[0,0]");
    source = replace(std::move(source), "[120, 120]", "[2, 2]");
    return source;
}

std::shared_ptr<const render::detail::OwnNativeEllipsePreparedAsset> prepared(
    const std::string& source) {
    return test::prepareOwnEllipseForTest(source);
}

render::detail::OwnNativeEllipseCreateResult create(const std::string& source) {
    return OwnNativeEllipseStream::create(prepared(source));
}

runtime::EvaluatedScene emit(OwnNativeEllipseStream& stream, std::size_t frame,
                              std::size_t width = 512, std::size_t height = 512) {
    auto result = stream.emit(frame, width, height);
    require(static_cast<bool>(result), "own stream emitted a scene");
    return std::move(*result.scene);
}

void sharedEvaluationHelpers() {
    const auto owner = prepared(staticSource());
    evaluation::PropertyEvaluator evaluator{owner->model};
    require(evaluator.valid(), "static own evaluator valid");
    evaluation::PropertyEvaluationWorkspace workspace;
    evaluator.prepare(workspace);
    const auto view = evaluator.evaluate(0.0, workspace);
    require(view, "static own evaluation view valid");
    model::MotionVec2Value position{-1, -1}, size{-1, -1};
    const auto& binding = owner->authored->binding;
    require(render::detail::resolveNativeEllipseVec2(
        *owner->model, view, binding.position, position)
        && position == model::MotionVec2Value{0, 0},
        "shared resolver reads static asset-reference position");
    require(render::detail::resolveNativeEllipseVec2(
        *owner->model, view, binding.size, size)
        && size == model::MotionVec2Value{2, 2},
        "shared resolver reads static asset-reference size");
    require(!render::detail::resolveNativeEllipseVec2(
        *owner->model, view, binding.color, position),
        "shared resolver rejects a non-Vec2 property");
    const model::MotionMatrix3x2Value world{1, 0, 0, 1, 256, 256};
    const auto transform = render::detail::nativeEllipseViewportTransform(
        world, 512, 512, 384, 256);
    require(transform && transform->m11 == 0.5F && transform->m22 == 0.5F
        && transform->dx == 192.0F && transform->dy == 128.0F,
        "shared aspect-fit transform uses exact nonsquare offset");
}

void creationAndSchema() {
    static_assert(!std::is_copy_constructible_v<OwnNativeEllipseStream>);
    static_assert(!std::is_move_constructible_v<OwnNativeEllipseStream>);
    const auto invalid = OwnNativeEllipseStream::create(nullptr);
    require(!invalid && invalid.code == OwnNativeEllipseCreateCode::InvalidPreparedAsset
        && !invalid.stream, "null sealed preparation rejected");
    const auto source = animatedSource();
    auto owner = prepared(source);
    auto created = OwnNativeEllipseStream::create(owner);
    require(created && created.code == OwnNativeEllipseCreateCode::Ready,
        "sealed own JSON creates stream");
    const auto scene = emit(*created.stream, 0);
    require(scene.sourceAssetHash == test::ownRawHash(source) && scene.sourceAssetHash != 0
        && scene.instanceId != 0 && !scene.assetHandle.valid()
        && !scene.instanceHandle.valid() && scene.evaluationSequence == 1
        && scene.frameIndex == 0 && scene.viewportWidth == 512 && scene.viewportHeight == 512,
        "independent source and stream metadata");
    require(scene.assetModelApplied && scene.assetModel == owner->model
        && scene.modelLayerCount == 2 && scene.modelNodeCount == 1
        && scene.modelGeometryCount == 1 && scene.modelPaintCount == 1,
        "frozen own model applied to scene");
    require(scene.layers.size() == 2 && scene.childLayerIndices == std::vector<std::uint32_t>{1}
        && scene.layers[0].modelLayer == model::makeId<model::LayerId>(0)
        && scene.layers[0].parentLayer == runtime::kInvalidSceneIndex
        && scene.layers[0].keyPath == "__" && scene.layers[0].visible
        && scene.layers[0].childCount == 1 && scene.layers[0].drawItemCount == 0
        && scene.layers[1].modelLayer == model::makeId<model::LayerId>(1)
        && scene.layers[1].parentLayer == 0 && scene.layers[1].keyPath == "Moving Circle"
        && scene.layers[1].visible && scene.layers[1].drawItemCount == 1
        && scene.layers[1].matte == runtime::MatteMode::None
        && scene.layers[1].opacity == 1 && scene.layers[1].maskCount == 0,
        "literal own root and shape rows with defaults");
    require(scene.drawItems.size() == 1 && scene.statistics.layerCount == 2
        && scene.statistics.visibleLayerCount == 2 && scene.statistics.drawItemCount == 1
        && scene.statistics.solidPaintCount == 1 && scene.statistics.maskCount == 0,
        "one visible draw and zero unsupported resources");
    const auto& item = scene.drawItems[0];
    require(item.modelDrawItem == model::makeId<model::DrawItemId>(0)
        && item.modelNode == model::makeId<model::NodeId>(0)
        && item.modelGeometry == model::makeId<model::GeometryId>(0)
        && item.modelPaint == model::makeId<model::PaintId>(0)
        && item.sourcePathNode == model::makeId<model::SourceNodeId>(3)
        && item.sourcePaintNode == model::makeId<model::SourceNodeId>(4)
        && item.sourcePathCount == 1 && item.sourcePathModifierFree
        && item.layerIndex == 1 && item.drawOrder == 0
        && item.fillRule == runtime::FillRule::Winding,
        "own draw and source identifiers");
    require(item.localGeometryAvailable && !item.localGeometryStaticCandidate
        && item.localPaintAvailable && item.localPaintStaticCandidate
        && !item.stroke.enabled && !item.localStroke.enabled
        && item.paint.kind == runtime::PaintKind::Solid
        && item.paint.solid.r == 20 && item.paint.solid.g == 183
        && item.paint.solid.b == 242 && item.paint.solid.a == 255
        && item.localPaint.kind == runtime::PaintKind::Solid
        && item.localPaint.solid.r == 20 && item.localPaint.solid.g == 183
        && item.localPaint.solid.b == 242 && item.localPaint.solid.a == 255
        && item.opacitySeparated && item.separatedOpacity == 1,
        "literal own paint, stroke and static classification");
    require(item.geometryOrigin == runtime::EvaluatedValueOrigin::InstanceEvaluated
        && !item.canonicalGeometry
        && item.paintOrigin == runtime::EvaluatedValueOrigin::AssetStatic
        && item.canonicalPaint && item.canonicalPaint->paint.solid.r == 20,
        "own model application resource classification");
    require(item.path.verbs.size() == 6 && item.path.points.size() == 13
        && item.path.verbs.front() == runtime::PathVerb::MoveTo
        && item.path.verbs.back() == runtime::PathVerb::Close
        && item.path.controlBounds.valid && item.path.controlBounds.left == 120
        && item.path.controlBounds.top == 196 && item.path.controlBounds.right == 240
        && item.path.controlBounds.bottom == 316
        && item.localToViewport.dx == 256 && item.localToViewport.dy == 256,
        "frame-zero ellipse geometry and layer transform");
    const auto fingerprints = runtime::computeSceneFingerprints(scene);
    require(scene.fingerprints.scene == fingerprints.scene
        && scene.fingerprints.topology == fingerprints.topology
        && scene.fingerprints.geometry == fingerprints.geometry
        && scene.fingerprints.paint == fingerprints.paint
        && scene.fingerprints.scene != 0 && scene.fingerprints.geometry != 0,
        "emitted fingerprints agree with the recorder");
}

void literalStaticFingerprints() {
    // These four literals come from the separately retained byte-layout/FNV
    // calculation in out/part26h/task-1/fix-round1/fingerprint_witness.py.
    // Inputs are authored/static scalar, path, paint and layer fields; no
    // expected value is computed from this scene or the product hash helper.
    auto created = create(staticSource());
    require(created, "static fingerprint witness stream ready");
    const auto scene = emit(*created.stream, 0);
    require(scene.fingerprints.topology == 0x25f9329c86cc4fa8ULL,
        "literal static scene topology fingerprint");
    require(scene.fingerprints.geometry == 0x116550a6f159e6cbULL,
        "literal static scene geometry fingerprint");
    require(scene.fingerprints.paint == 0xbe69be34787a3f86ULL,
        "literal static scene paint fingerprint");
    require(scene.fingerprints.scene == 0xc030aaf03448ddddULL,
        "literal static full scene fingerprint");
}

void sequenceAndSeeking() {
    auto created = create(animatedSource());
    require(created, "animated stream ready");
    auto first = emit(*created.stream, 0);
    require(first.changes.firstEvaluation, "first scene marks first evaluation");
    auto repeated = emit(*created.stream, 0);
    require(repeated.evaluationSequence == 2 && !repeated.changes.firstEvaluation
        && !repeated.changes.topologyChanged && !repeated.changes.geometryChanged
        && !repeated.changes.paintChanged && !repeated.changes.visualChanged
        && repeated.fingerprints.scene == first.fingerprints.scene
        && repeated.fingerprints.geometry == first.fingerprints.geometry
        && repeated.fingerprints.paint == first.fingerprints.paint,
        "repeated frame has fresh sequence and no change");
    const auto bad = created.stream->emit(30, 0, 512);
    require(!bad && bad.code == OwnNativeEllipseFrameCode::InvalidViewport && !bad.scene,
        "zero viewport is typed failure");
    auto last = emit(*created.stream, 999);
    require(last.evaluationSequence == 4 && last.frameIndex == 60
        && last.changes.geometryChanged && last.changes.visualChanged
        && !last.changes.firstEvaluation && !last.changes.paintChanged
        && last.fingerprints.geometry != repeated.fingerprints.geometry
        && last.fingerprints.paint == repeated.fingerprints.paint
        && last.drawItems[0].path.controlBounds.left == 272,
        "failed attempt advances sequence but not history; frame clamps to sixty");
    auto back = emit(*created.stream, 0);
    require(back.evaluationSequence == 5 && back.changes.geometryChanged
        && back.drawItems[0].path.controlBounds.left == 120
        && first.drawItems[0].path.controlBounds.left == 120,
        "reverse seek preserves old snapshot");
    auto same = emit(*created.stream, 0);
    require(!same.changes.visualChanged && !same.changes.geometryChanged,
        "history tracks prior successful scene");
}

void viewportsAndVisibility() {
    auto created = create(animatedSource());
    require(created, "viewport stream ready");
    struct View { std::size_t width, height; float left, top, right, bottom; };
    constexpr std::array views{
        View{512, 512, 120, 196, 240, 316},
        View{256, 256, 60, 98, 120, 158},
        View{384, 256, 124, 98, 184, 158},
        View{256, 384, 60, 162, 120, 222},
    };
    for (const auto& view : views) {
        const auto scene = emit(*created.stream, 0, view.width, view.height);
        const auto& bounds = scene.drawItems[0].path.controlBounds;
        require(bounds.left == view.left && bounds.top == view.top
            && bounds.right == view.right && bounds.bottom == view.bottom,
            "aspect-fit viewport path bounds");
    }
    auto interval = replace(animatedSource(), "\"ip\": 0,\n      \"op\": 61,\n      \"st\": 0",
        "\"ip\": 10,\n      \"op\": 20,\n      \"st\": 0");
    auto active = create(interval);
    require(active, "interval stream ready");
    for (const std::size_t frame : {0U, 9U, 20U, 60U}) {
        const auto scene = emit(*active.stream, frame);
        require(scene.layers[0].visible && !scene.layers[1].visible
            && scene.layers[1].drawItemCount == 0 && scene.drawItems.empty()
            && scene.statistics.visibleLayerCount == 1
            && scene.assetModel && scene.modelGeometryCount == 1,
            "half-open inactive interval retains model but emits no draw");
    }
    for (const std::size_t frame : {10U, 19U}) {
        const auto scene = emit(*active.stream, frame);
        require(scene.layers[1].visible && scene.drawItems.size() == 1,
            "half-open active interval emits draw");
    }
}

void staticAndTgs() {
    const auto source = staticSource();
    auto created = create(source);
    require(created, "static stream ready");
    const auto first = emit(*created.stream, 0);
    const auto last = emit(*created.stream, 60);
    require(first.drawItems.size() == 1 && first.drawItems[0].localGeometryStaticCandidate
        && first.drawItems[0].geometryOrigin == runtime::EvaluatedValueOrigin::AssetStatic
        && first.drawItems[0].canonicalGeometry
        && last.drawItems[0].canonicalGeometry == first.drawItems[0].canonicalGeometry
        && !last.changes.geometryChanged && !last.changes.paintChanged,
        "static geometry and paint share frozen model resources");
    auto collapsed = replace(animatedSource(), "[120, 120]", "[1e-45, 120]");
    auto animated = create(collapsed);
    require(animated, "animated tiny path may be prepared");
    const auto tiny = emit(*animated.stream, 0);
    require(tiny.drawItems.size() == 1 && tiny.drawItems[0].path.verbs.empty(),
        "animated tiny path may collapse to empty");
    auto staticTiny = replace(test::staticEllipseFixture(animatedSource()),
        "[120, 120]", "[1e-45, 120]");
    const auto parsed = formats::detail::readOwnJson(staticTiny);
    require(parsed, "static tiny JSON parsed");
    const auto modelResult = runtime::detail::buildOwnNativeEllipseModel(*parsed.document);
    require(modelResult, "static tiny authored model admitted");
    const auto resourceResult = render::detail::prepareOwnNativeEllipseAsset(modelResult.prepared);
    require(!resourceResult && resourceResult.code == render::detail::OwnNativeEllipsePrepareCode::ResourceConstructionFailed,
        "static tiny geometry remains rejected at preparation");
    const auto tgs = formats::decodeTgsFile(
        std::filesystem::path{AVEMOTION_TGS_DIR} / "telegram_sticker_basic.tgs");
    require(tgs, "TGS decoded");
    auto sticker = create(tgs.json);
    require(sticker && emit(*sticker.stream, 30).drawItems.size() == 1,
        "TGS decoded bytes traverse own model and stream");
}

void counterBoundaries() {
    std::atomic<std::uint64_t> identity{std::numeric_limits<std::uint64_t>::max() - 1};
    const auto last = render::detail::tryNextOwnStreamIdentity(identity);
    require(last && *last == std::numeric_limits<std::uint64_t>::max(),
        "last identity is issued");
    require(!render::detail::tryNextOwnStreamIdentity(identity)
        && identity.load() == std::numeric_limits<std::uint64_t>::max(),
        "identity exhaustion leaves counter unchanged");
    std::uint64_t sequence = std::numeric_limits<std::uint64_t>::max() - 1;
    require(render::detail::tryAdvanceOwnStreamSequence(sequence)
        && sequence == std::numeric_limits<std::uint64_t>::max(),
        "last attempt sequence is issued");
    require(!render::detail::tryAdvanceOwnStreamSequence(sequence)
        && sequence == std::numeric_limits<std::uint64_t>::max(),
        "sequence exhaustion does not wrap");
}

void countersAndIdentity() {
    counterBoundaries();
    const auto source = animatedSource();
    auto owner = prepared(source);
    auto a = OwnNativeEllipseStream::create(owner);
    auto b = OwnNativeEllipseStream::create(owner);
    auto c = OwnNativeEllipseStream::create(prepared(source));
    auto d = OwnNativeEllipseStream::create(prepared(staticSource()));
    require(a && b && c && d, "four own streams created");
    const auto ai = emit(*a.stream, 0).instanceId;
    const auto bi = emit(*b.stream, 0).instanceId;
    const auto ci = emit(*c.stream, 0).instanceId;
    const auto di = emit(*d.stream, 0).instanceId;
    require(ai != bi && ai != ci && ai != di && bi != ci && bi != di && ci != di,
        "same owner, equal source and distinct source all receive distinct IDs");
    a.stream.reset();
    auto e = OwnNativeEllipseStream::create(owner);
    require(e && emit(*e.stream, 0).instanceId != ai,
        "destroyed stream identity is never recycled");
}

void concurrentStreams() {
    auto owner = prepared(animatedSource());
    std::array<std::array<std::uint64_t, 8>, 4> identities{};
    std::array<std::string, 4> errors{};
    std::array<std::thread, 4> workers;
    for (std::size_t worker = 0; worker < workers.size(); ++worker) {
        workers[worker] = std::thread([&, worker] {
            try {
                for (std::size_t index = 0; index < 8; ++index) {
                    auto created = OwnNativeEllipseStream::create(owner);
                    require(created, "concurrent stream created");
                    const auto scene = emit(*created.stream, index * 7);
                    require(scene.evaluationSequence == 1 && scene.drawItems.size() == 1,
                        "concurrent worker owns evaluator and workspace");
                    identities[worker][index] = scene.instanceId;
                }
            } catch (const std::exception& error) { errors[worker] = error.what(); }
        });
    }
    for (auto& worker : workers) worker.join();
    std::vector<std::uint64_t> seen;
    for (std::size_t worker = 0; worker < workers.size(); ++worker) {
        require(errors[worker].empty(), "concurrent worker completed");
        for (const auto id : identities[worker]) {
            require(id != 0, "concurrent identity nonzero");
            for (const auto earlier : seen) require(id != earlier, "concurrent identity unique");
            seen.push_back(id);
        }
    }
}

void plannerAndLifetime() {
    auto owner = prepared(animatedSource());
    auto a = OwnNativeEllipseStream::create(owner);
    auto b = OwnNativeEllipseStream::create(owner);
    require(a && b, "planner streams ready");
    render::MotionRenderPlanner planner;
    auto a0 = emit(*a.stream, 0);
    auto b0 = emit(*b.stream, 0);
    const auto aid = a0.instanceId;
    auto pa0 = planner.build(a0);
    auto pb0 = planner.build(b0);
    require(pa0 && pb0 && pa0.plan.firstPlan && pb0.plan.firstPlan
        && pa0.plan.drawItems.size() == 1 && pb0.plan.drawItems.size() == 1,
        "one planner gives same-source A and B independent first plans");
    require(pa0.plan.drawItems[0].geometry.scope == render::ResourceIdentityScope::Instance
        && pb0.plan.drawItems[0].geometry.scope == render::ResourceIdentityScope::Instance
        && pa0.plan.drawItems[0].geometry != pb0.plan.drawItems[0].geometry
        && pa0.plan.drawItems[0].paint.scope == render::ResourceIdentityScope::Asset
        && pa0.plan.drawItems[0].paint == pb0.plan.drawItems[0].paint,
        "animated geometry partitions by own ID while paint shares source");
    auto pa1 = planner.build(emit(*a.stream, 0));
    auto pb1 = planner.build(emit(*b.stream, 30));
    require(pa1 && pb1 && !pa1.plan.firstPlan && !pb1.plan.firstPlan
        && pa1.plan.geometryUpdates.empty() && pb1.plan.geometryUpdates.size() == 1
        && pa1.plan.stamp.planSequence == 2 && pb1.plan.stamp.planSequence == 2,
        "interleaved repeated and advanced plans keep independent state");
    auto a2 = emit(*a.stream, 60);
    auto pa2 = planner.build(a2);
    require(pa2 && pa2.plan.geometryUpdates.size() == 1,
        "advanced A causes own geometry update");
    const auto stale = planner.build(a0);
    require(!stale && stale.error.code == render::RenderPlanErrorCode::StaleSnapshot,
        "old A sequence is stale");
    auto equal = planner.build(a2);
    require(equal && !equal.plan.firstPlan,
        "equal accepted sequence may rebuild");
    auto saved = pa0.plan;
    auto alias = saved.sourceScene->drawItems[0].canonicalPaint;
    planner.forgetInstance(aid);
    auto rebuilt = planner.build(a0);
    require(rebuilt && rebuilt.plan.firstPlan && rebuilt.plan.stamp.planSequence == 1,
        "forget resets own stream planner state");
    a.stream.reset(); b.stream.reset(); owner.reset();
    a0 = {}; b0 = {}; a2 = {};
    pa0 = {}; pb0 = {}; pa1 = {}; pb1 = {}; pa2 = {}; equal = {}; rebuilt = {};
    require(saved.sourceScene && saved.sourceScene->assetModel
        && saved.sourceScene->assetModelApplied && alias
        && alias->paint.solid.r == 20 && saved.drawItems.size() == 1,
        "retained plan, model and alias survive external owner release");
    require(saved.sourceScene->drawItems[0].path.controlBounds.left == 120,
        "old retained plan remains usable after forget and rebuild");

    auto staticA = create(staticSource());
    auto staticB = create(staticSource());
    auto otherSource = replace(staticSource(), "[0.08, 0.72, 0.95, 1]", "[0.5, 0.1, 0.999, 1]");
    auto staticC = create(otherSource);
    require(staticA && staticB && staticC, "static source streams ready");
    auto sa = planner.build(emit(*staticA.stream, 0));
    auto sb = planner.build(emit(*staticB.stream, 0));
    auto sc = planner.build(emit(*staticC.stream, 0));
    require(sa && sb && sc
        && sa.plan.drawItems[0].geometry == sb.plan.drawItems[0].geometry
        && sa.plan.drawItems[0].paint == sb.plan.drawItems[0].paint
        && sa.plan.drawItems[0].geometry != sc.plan.drawItems[0].geometry
        && sa.plan.drawItems[0].paint != sc.plan.drawItems[0].paint,
        "static cache keys share same source and partition different source");
}

void collisionCharacterization() {
    auto created = create(animatedSource());
    require(created, "collision source ready");
    auto first = emit(*created.stream, 0);
    render::MotionRenderPlanner planner;
    const auto accepted = planner.build(first);
    require(accepted, "first scene accepted");
    auto duplicate = first;
    duplicate.evaluationSequence = 0;
    require(planner.build(duplicate).error.code == render::RenderPlanErrorCode::StaleSnapshot,
        "synthetic duplicate raw ID collides in planner history");
    auto packed = first;
    packed.instanceId += 10;
    packed.instanceHandle = {static_cast<std::uint32_t>(first.instanceId), 1};
    auto plain = first;
    plain.instanceId = packed.instanceHandle.packed();
    plain.evaluationSequence = 0;
    const auto handleFirst = planner.build(packed);
    require(handleFirst && handleFirst.plan.firstPlan, "synthetic Runtime handle accepted");
    require(planner.build(plain).error.code == render::RenderPlanErrorCode::StaleSnapshot,
        "packed Runtime handle can collide with raw own ID");
}

} // namespace

int main(int argc, char** argv) {
    try {
        if (argc == 2 && std::string_view{argv[1]} == "--helpers") {
            sharedEvaluationHelpers();
            std::cout << "native ellipse evaluation helpers: all checks passed\n";
            return 0;
        }
        if (argc == 2 && std::string_view{argv[1]} == "--counters") {
            counterBoundaries();
            std::cout << "own stream counter boundaries: all checks passed\n";
            return 0;
        }
        creationAndSchema();
        sharedEvaluationHelpers();
        literalStaticFingerprints();
        sequenceAndSeeking();
        viewportsAndVisibility();
        staticAndTgs();
        countersAndIdentity();
        concurrentStreams();
        plannerAndLifetime();
        collisionCharacterization();
        std::cout << "own native ellipse stream: all checks passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "own native ellipse stream: " << error.what() << '\n';
        return 1;
    }
}
