#include "OwnNativeEllipseStream.hpp"
#include "OwnNativeEllipseStreamCounters.hpp"

#include "AssetModelBuilder.hpp"
#include "NativeEllipseEvaluationHelpers.hpp"
#include "NativeEllipsePathMaterializer.hpp"
#include "PrimitivePathGenerator.hpp"
#include "avemotion/runtime/RecordingBackend.hpp"

#include <algorithm>
#include <limits>
#include <utility>

namespace avemotion::render::detail {
namespace {

// One linked allocator serves the own-only planner/backend domain. That domain
// must not mix reference Runtime or caller-selected legacy identities, and must
// be destroyed before this allocator's module is unloaded. A stream is single-
// writer; separate streams may share an immutable prepared owner concurrently.
std::atomic<std::uint64_t> lastOwnStreamIdentity{0};

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
        || prepared->authored->binding.groups.empty()
        || prepared->model->nodes.size() != prepared->authored->binding.groups.size()
        || prepared->model->geometries.size() != prepared->authored->binding.groups.size()
        || prepared->model->paints.size() != prepared->authored->binding.groups.size()
        || std::any_of(prepared->model->paints.begin(), prepared->model->paints.end(),
            [](const auto& paint) { return !paint.staticValue; })
        || !runtime::detail::bindOwnPrimitiveModel(*prepared->authored->input, *prepared->model)) {
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
    const auto count = static_cast<std::uint32_t>(authored.binding.groups.size());
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
    scene.layers.push_back(makeLayer(model.layers[1], active, active ? count : 0U));
    scene.childLayerIndices.push_back(1);
    scene.statistics.layerCount = 2;
    scene.statistics.visibleLayerCount = active ? 2 : 1;

    if (active) for (std::uint32_t slot = 0; slot < count; ++slot) {
        const auto g = count - 1U - slot;
        const auto& binding = authored.binding.groups[g];
        const auto& values = authored.values.groups[g];
        model::MotionVec2Value position, size;
        float roundness = 0;
        if (!resolveNativeEllipseVec2(model, view, binding.position, position)
            || !resolveNativeEllipseVec2(model, view, binding.size, size)
            || (binding.roundness && !resolveNativeEllipseScalar(model, view, *binding.roundness, roundness))
            || binding.group.index() >= view.nodeTransforms.size()) {
            return {OwnNativeEllipseFrameCode::EvaluationFailed,
                    "bound primitive property or transform unavailable", std::nullopt};
        }
        const auto& group = view.nodeTransforms[binding.group.index()];
        if (group.node != binding.group || !group.worldSupported()) {
            return {OwnNativeEllipseFrameCode::EvaluationFailed,
                    "group world transform unavailable", std::nullopt};
        }
        const auto transform = nativeEllipseViewportTransform(group.worldMatrix,
            model.logicalWidth, model.logicalHeight, width, height);
        if (!transform) {
            return {OwnNativeEllipseFrameCode::UnsupportedNumericOutput,
                    "non-finite viewport transform", std::nullopt};
        }
        const auto* sourcePrimitive = model.sourceNode(binding.primitive);
        if (!sourcePrimitive) {
            return {OwnNativeEllipseFrameCode::EvaluationFailed,
                    "bound primitive node unavailable", std::nullopt};
        }
        const auto primitive = sourcePrimitive->kind == model::SourceNodeKind::Rectangle
            ? generateRectanglePath(position, size, roundness, sourcePrimitive->pathDirection)
            : generateEllipsePath(position, size, sourcePrimitive->pathDirection);
        runtime::EvaluatedDrawItem item;
        const auto& node = model.nodes[slot];
        item.modelDrawItem = node.drawItem;
        item.modelNode = node.id;
        item.modelGeometry = node.geometry;
        item.modelPaint = node.paint;
        item.sourcePathNode = binding.primitive;
        item.sourcePaintNode = binding.fill;
        item.sourcePathCount = 1;
        item.sourcePathModifierFree = true;
        item.layerIndex = 1;
        item.drawOrder = node.drawOrder;
        item.fillRule = runtime::FillRule::Winding;
        item.localGeometryAvailable = true;
        item.localGeometryStaticCandidate = !values.position.animated && !values.size.animated
            && !(values.roundness && values.roundness->animated);
        item.localToViewport = *transform;
        item.localPaintAvailable = true;
        item.localPaintStaticCandidate = true;
        item.paint = model.paints[slot].staticValue->paint;
        item.localPaint = model.paints[slot].staticValue->paint;
        item.opacitySeparated = true;
        item.separatedOpacity = 1.0F;
        if (!materializeNativeEllipsePath(primitive, nullptr, item.localPath)
            || !materializeNativeEllipsePath(primitive, &*transform, item.path)) {
            return {OwnNativeEllipseFrameCode::UnsupportedNumericOutput,
                    "non-finite primitive path", std::nullopt};
        }
        ++scene.statistics.drawItemCount;
        ++scene.statistics.solidPaintCount;
        scene.statistics.pathVerbCount += item.path.verbs.size();
        scene.statistics.pathPointCount += item.path.points.size();
        const auto& bounds = item.path.controlBounds;
        if (bounds.valid) {
            if (!scene.controlBounds.valid) scene.controlBounds = bounds;
            else {
                scene.controlBounds.left = std::min(scene.controlBounds.left, bounds.left);
                scene.controlBounds.top = std::min(scene.controlBounds.top, bounds.top);
                scene.controlBounds.right = std::max(scene.controlBounds.right, bounds.right);
                scene.controlBounds.bottom = std::max(scene.controlBounds.bottom, bounds.bottom);
            }
        }
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
