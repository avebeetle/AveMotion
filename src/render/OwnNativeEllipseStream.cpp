#include "OwnNativeEllipseStream.hpp"
#include "OwnNativeEllipseStreamCounters.hpp"

#include "AssetModelBuilder.hpp"
#include "NativeEllipsePathMaterializer.hpp"
#include "PrimitivePathGenerator.hpp"
#include "avemotion/runtime/RecordingBackend.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>

namespace avemotion::render::detail {
namespace {

// One linked allocator serves the own-only planner/backend domain. That domain
// must not mix reference Runtime or caller-selected legacy identities, and must
// be destroyed before this allocator's module is unloaded. A stream is single-
// writer; separate streams may share an immutable prepared owner concurrently.
std::atomic<std::uint64_t> lastOwnStreamIdentity{0};

bool finite(const runtime::AffineTransform& value) noexcept {
    return std::isfinite(value.m11) && std::isfinite(value.m12)
        && std::isfinite(value.m21) && std::isfinite(value.m22)
        && std::isfinite(value.dx) && std::isfinite(value.dy);
}

bool resolveVec2(const model::MotionAssetModel& model,
                 const evaluation::PropertyEvaluationView& view,
                 model::PropertyId id, model::MotionVec2Value& value) noexcept {
    const auto* property = model.property(id);
    if (!property || property->valueType != model::PropertyValueType::Vec2
        || id.index() >= view.properties.size()) return false;
    const auto& evaluated = view.properties[id.index()];
    if (evaluated.id != id || evaluated.value.type != model::PropertyValueType::Vec2)
        return false;
    if (evaluated.value.storage == evaluation::PropertyStorageKind::Materialized) {
        value = evaluated.value.vec2;
    } else if (evaluated.value.storage == evaluation::PropertyStorageKind::AssetReference) {
        const auto reference = evaluated.value.assetReference;
        if (reference.type != model::PropertyValueType::Vec2
            || reference.index >= model.vec2Values.size()) return false;
        value = model.vec2Values[reference.index];
    } else return false;
    return std::isfinite(value.x) && std::isfinite(value.y);
}

runtime::EvaluatedLayer makeLayer(const model::MotionLayerRecord& source,
                                  bool visible, std::uint32_t drawCount) {
    runtime::EvaluatedLayer layer;
    layer.modelLayer = source.id;
    layer.parentLayer = source.parent.valid() ? source.parent.index()
        : runtime::kInvalidSceneIndex;
    layer.firstChildReference = source.children.first;
    layer.childCount = source.children.count;
    layer.firstDrawItem = 0;
    layer.drawItemCount = drawCount;
    layer.keyPath = source.debugName;
    layer.visible = visible;
    layer.matte = source.matte;
    return layer;
}

} // namespace

std::optional<std::uint64_t> tryNextOwnStreamIdentity(
    std::atomic<std::uint64_t>& last) noexcept {
    auto current = last.load(std::memory_order_relaxed);
    for (;;) {
        if (current == std::numeric_limits<std::uint64_t>::max()) return std::nullopt;
        if (last.compare_exchange_weak(current, current + 1U,
                                      std::memory_order_relaxed,
                                      std::memory_order_relaxed)) {
            return current + 1U;
        }
    }
}

bool tryAdvanceOwnStreamSequence(std::uint64_t& last) noexcept {
    if (last == std::numeric_limits<std::uint64_t>::max()) return false;
    ++last;
    return true;
}

OwnNativeEllipseStream::OwnNativeEllipseStream(
    std::shared_ptr<const OwnNativeEllipsePreparedAsset> prepared,
    std::uint64_t identity)
    : prepared_(std::move(prepared)), identity_(identity), evaluator_(prepared_->model) {}

OwnNativeEllipseStream::~OwnNativeEllipseStream() = default;

OwnNativeEllipseCreateResult OwnNativeEllipseStream::create(
    std::shared_ptr<const OwnNativeEllipsePreparedAsset> prepared) {
    if (!prepared || !prepared->authored || !prepared->authored->input
        || !prepared->authored->model || !prepared->model
        || prepared->model->totalFrames == 0 || prepared->model->logicalWidth == 0
        || prepared->model->logicalHeight == 0 || prepared->model->layers.size() != 2
        || prepared->model->nodes.size() != 1 || prepared->model->geometries.size() != 1
        || prepared->model->paints.size() != 1
        || !prepared->model->paints[0].staticValue) {
        return {OwnNativeEllipseCreateCode::InvalidPreparedAsset,
                "incomplete own ellipse preparation", nullptr};
    }
    if (prepared->model->sourceAssetHash == 0
        || prepared->model->sourceAssetHash != prepared->authored->model->sourceAssetHash) {
        return {OwnNativeEllipseCreateCode::InvalidSourceIdentity,
                "invalid own ellipse source identity", nullptr};
    }
    const auto identity = tryNextOwnStreamIdentity(lastOwnStreamIdentity);
    if (!identity) {
        return {OwnNativeEllipseCreateCode::IdentityExhausted,
                "own stream identities exhausted", nullptr};
    }
    auto stream = std::unique_ptr<OwnNativeEllipseStream>(
        new OwnNativeEllipseStream(std::move(prepared), *identity));
    if (!stream->evaluator_.valid()) {
        return {OwnNativeEllipseCreateCode::EvaluationPreparationFailed,
                std::string(stream->evaluator_.errorMessage()), nullptr};
    }
    stream->evaluator_.prepare(stream->workspace_);
    return {OwnNativeEllipseCreateCode::Ready, {}, std::move(stream)};
}

OwnNativeEllipseFrameResult OwnNativeEllipseStream::emit(
    std::size_t frame, std::size_t width, std::size_t height) {
    if (!tryAdvanceOwnStreamSequence(attemptSequence_)) {
        return {OwnNativeEllipseFrameCode::SequenceExhausted,
                "own stream emission sequence exhausted", std::nullopt};
    }
    if (width == 0 || height == 0) {
        return {OwnNativeEllipseFrameCode::InvalidViewport, "zero viewport", std::nullopt};
    }
    const auto& authored = *prepared_->authored;
    const auto& input = *authored.input;
    const auto& model = *prepared_->model;
    const auto& binding = authored.binding;
    frame = std::min(frame, model.totalFrames - 1U);
    const auto view = evaluator_.evaluate(static_cast<double>(frame), workspace_);
    if (!view) {
        return {OwnNativeEllipseFrameCode::EvaluationFailed,
                "property evaluation failed", std::nullopt};
    }

    runtime::EvaluatedScene scene;
    scene.sourceAssetHash = model.sourceAssetHash;
    scene.instanceId = identity_;
    scene.evaluationSequence = attemptSequence_;
    scene.frameIndex = frame;
    scene.viewportWidth = width;
    scene.viewportHeight = height;
    scene.modelLayerCount = static_cast<std::uint32_t>(model.layers.size());
    scene.modelNodeCount = static_cast<std::uint32_t>(model.nodes.size());
    scene.modelGeometryCount = static_cast<std::uint32_t>(model.geometries.size());
    scene.modelPaintCount = static_cast<std::uint32_t>(model.paints.size());
    const bool active = frame >= input.layerInFrame && frame < input.layerOutFrame;
    scene.layers.push_back(makeLayer(model.layers[0], true, 0));
    scene.layers.push_back(makeLayer(model.layers[1], active, active ? 1U : 0U));
    scene.childLayerIndices.push_back(1);
    scene.statistics.layerCount = 2;
    scene.statistics.visibleLayerCount = active ? 2 : 1;

    if (active) {
        model::MotionVec2Value position, size;
        if (!resolveVec2(model, view, binding.position, position)
            || !resolveVec2(model, view, binding.size, size)
            || binding.group.index() >= view.nodeTransforms.size()) {
            return {OwnNativeEllipseFrameCode::EvaluationFailed,
                    "bound ellipse property or transform unavailable", std::nullopt};
        }
        const auto& group = view.nodeTransforms[binding.group.index()];
        if (group.node != binding.group || !group.worldSupported()) {
            return {OwnNativeEllipseFrameCode::EvaluationFailed,
                    "group world transform unavailable", std::nullopt};
        }
        const float scale = std::min(float(width) / float(model.logicalWidth),
                                     float(height) / float(model.logicalHeight));
        const float tx = (float(width) - float(model.logicalWidth) * scale) / 2.0F;
        const float ty = (float(height) - float(model.logicalHeight) * scale) / 2.0F;
        const auto& world = group.worldMatrix;
        const runtime::AffineTransform transform{world.m11 * scale, world.m12 * scale,
            world.m21 * scale, world.m22 * scale, world.dx * scale + tx,
            world.dy * scale + ty};
        if (!finite(transform)) {
            return {OwnNativeEllipseFrameCode::UnsupportedNumericOutput,
                    "non-finite viewport transform", std::nullopt};
        }
        const auto* ellipse = model.sourceNode(binding.ellipse);
        if (!ellipse) {
            return {OwnNativeEllipseFrameCode::EvaluationFailed,
                    "bound ellipse node unavailable", std::nullopt};
        }
        const auto primitive = generateEllipsePath(position, size, ellipse->pathDirection);
        runtime::EvaluatedDrawItem item;
        const auto& node = model.nodes[0];
        item.modelDrawItem = node.drawItem;
        item.modelNode = node.id;
        item.modelGeometry = node.geometry;
        item.modelPaint = node.paint;
        item.sourcePathNode = binding.ellipse;
        item.sourcePaintNode = binding.fill;
        item.sourcePathCount = 1;
        item.sourcePathModifierFree = true;
        item.layerIndex = 1;
        item.drawOrder = node.drawOrder;
        item.fillRule = runtime::FillRule::Winding;
        item.localGeometryAvailable = true;
        item.localGeometryStaticCandidate = !authored.values.animated;
        item.localToViewport = transform;
        item.localPaintAvailable = true;
        item.localPaintStaticCandidate = true;
        item.paint = model.paints[0].staticValue->paint;
        item.localPaint = model.paints[0].staticValue->paint;
        item.opacitySeparated = true;
        item.separatedOpacity = 1.0F;
        if (!materializeNativeEllipsePath(primitive, nullptr, item.localPath)
            || !materializeNativeEllipsePath(primitive, &transform, item.path)) {
            return {OwnNativeEllipseFrameCode::UnsupportedNumericOutput,
                    "non-finite ellipse path", std::nullopt};
        }
        scene.statistics.drawItemCount = 1;
        scene.statistics.solidPaintCount = 1;
        scene.statistics.pathVerbCount = item.path.verbs.size();
        scene.statistics.pathPointCount = item.path.points.size();
        scene.controlBounds = item.path.controlBounds;
        scene.drawItems.push_back(std::move(item));
    }

    const auto applied = model::detail::applyAssetModel(prepared_->model, scene);
    if (!applied) {
        return {OwnNativeEllipseFrameCode::ModelApplicationFailed, applied.error, std::nullopt};
    }
    scene.fingerprints = runtime::computeSceneFingerprints(scene);
    scene.changes.firstEvaluation = !hasPrevious_;
    if (hasPrevious_) {
        scene.changes.topologyChanged = scene.fingerprints.topology != previous_.topology;
        scene.changes.geometryChanged = scene.fingerprints.geometry != previous_.geometry;
        scene.changes.paintChanged = scene.fingerprints.paint != previous_.paint;
        scene.changes.visualChanged = scene.fingerprints.scene != previous_.scene;
    }
    previous_ = scene.fingerprints;
    hasPrevious_ = true;
    return {OwnNativeEllipseFrameCode::Emitted, {}, std::move(scene)};
}

} // namespace avemotion::render::detail
