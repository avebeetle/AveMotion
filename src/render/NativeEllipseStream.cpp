#include "NativeEllipseStream.hpp"
#include "NativeEllipsePathMaterializer.hpp"
#include "NativeEllipseEvaluationHelpers.hpp"
#include "PrimitivePathGenerator.hpp"
#include "AssetModelBuilder.hpp"
#include "avemotion/core/Hash.hpp"
#include "avemotion/runtime/RecordingBackend.hpp"

#include <algorithm>
#include <limits>
#include <utility>

namespace avemotion::render::detail {
namespace {

runtime::EvaluatedLayer makeLayer(const runtime::detail::NativeEllipseLayerSlot& slot,
                                  bool visible, std::uint32_t drawCount) {
    runtime::EvaluatedLayer layer;
    layer.modelLayer = slot.id;
    layer.parentLayer = slot.parentIndex;
    layer.firstChildReference = slot.firstChildReference;
    layer.childCount = slot.childCount;
    layer.firstDrawItem = slot.firstDrawItem;
    layer.drawItemCount = drawCount;
    layer.keyPath = slot.keyPath;
    layer.visible = visible;
    return layer;
}

} // namespace

NativeEllipseStream::NativeEllipseStream(
    std::shared_ptr<const runtime::detail::NativeEllipseCertificate> certificate,
    std::uint64_t instanceId)
    : certificate_(std::move(certificate)), instanceId_(instanceId),
      evaluator_(certificate_->model) {}

NativeEllipseStream::~NativeEllipseStream() = default;

NativeEllipseCreateResult NativeEllipseStream::create(
    std::shared_ptr<const runtime::detail::NativeEllipseCertificate> certificate,
    std::uint64_t instanceId) {
    if (!certificate || !certificate->input || !certificate->model
        || !certificate->matchesAsset(certificate->asset)
        || certificate->slot.totalFrames == 0
        || certificate->slot.width == 0 || certificate->slot.height == 0) {
        return {NativeEllipseCreateCode::InvalidCertificate, "invalid certificate", nullptr};
    }
    if (instanceId == 0) {
        return {NativeEllipseCreateCode::InvalidIdentity, "zero private identity", nullptr};
    }
    auto stream = std::unique_ptr<NativeEllipseStream>(
        new NativeEllipseStream(std::move(certificate), instanceId));
    if (!stream->evaluator_.valid()) {
        return {NativeEllipseCreateCode::EvaluationPreparationFailed,
                std::string(stream->evaluator_.errorMessage()), nullptr};
    }
    stream->evaluator_.prepare(stream->workspace_);
    return {NativeEllipseCreateCode::Ready, {}, std::move(stream)};
}

NativeEllipseFrameResult NativeEllipseStream::emit(
    std::size_t frame, std::size_t width, std::size_t height) {
    ++attemptSequence_;
    if (width == 0 || height == 0)
        return {NativeEllipseFrameCode::InvalidViewport, "zero viewport", std::nullopt};
    const auto& slot = certificate_->slot;
    const auto& model = *certificate_->model;
    frame = std::min(frame, slot.totalFrames - 1U);
    const auto view = evaluator_.evaluate(static_cast<double>(frame), workspace_);
    if (!view)
        return {NativeEllipseFrameCode::EvaluationFailed, "property evaluation failed", std::nullopt};

    runtime::EvaluatedScene scene;
    scene.sourceAssetHash = slot.sourceHash;
    scene.assetHandle = slot.assetHandle;
    scene.instanceId = instanceId_;
    scene.evaluationSequence = attemptSequence_;
    scene.frameIndex = frame;
    scene.viewportWidth = width;
    scene.viewportHeight = height;
    scene.modelLayerCount = slot.modelLayerCount;
    scene.modelNodeCount = slot.modelNodeCount;
    scene.modelGeometryCount = slot.modelGeometryCount;
    scene.modelPaintCount = slot.modelPaintCount;
    const bool active = frame >= slot.activeFirstFrame && frame < slot.activeEndFrame;
    scene.layers.push_back(makeLayer(slot.root, true, 0));
    scene.layers.push_back(makeLayer(slot.shape, active, active ? 1U : 0U));
    scene.childLayerIndices.push_back(1);
    scene.statistics.layerCount = 2;
    scene.statistics.visibleLayerCount = active ? 2 : 1;

    if (active) {
        model::MotionVec2Value position, size;
        if (!resolveNativeEllipseVec2(model, view, slot.binding.position, position)
            || !resolveNativeEllipseVec2(model, view, slot.binding.size, size)
            || slot.binding.group.index() >= view.nodeTransforms.size()) {
            return {NativeEllipseFrameCode::EvaluationFailed,
                    "bound ellipse property or transform unavailable", std::nullopt};
        }
        const auto& group = view.nodeTransforms[slot.binding.group.index()];
        if (group.node != slot.binding.group || !group.worldSupported()) {
            return {NativeEllipseFrameCode::EvaluationFailed,
                    "group world transform unavailable", std::nullopt};
        }
        const auto transform = nativeEllipseViewportTransform(group.worldMatrix,
            model.logicalWidth, model.logicalHeight, width, height);
        if (!transform) {
            return {NativeEllipseFrameCode::UnsupportedNumericOutput,
                    "non-finite viewport transform", std::nullopt};
        }
        const auto* ellipse = model.sourceNode(slot.binding.ellipse);
        if (!ellipse) {
            return {NativeEllipseFrameCode::EvaluationFailed,
                    "bound ellipse node unavailable", std::nullopt};
        }
        const auto primitive = generateEllipsePath(position, size, ellipse->pathDirection);
        runtime::EvaluatedDrawItem item;
        const auto& facts = slot.draw;
        item.modelDrawItem = facts.draw;
        item.modelNode = facts.node;
        item.modelGeometry = facts.geometry;
        item.modelPaint = facts.paint;
        item.sourcePathNode = facts.sourcePath;
        item.sourcePaintNode = facts.sourcePaint;
        item.sourcePathCount = facts.sourcePathCount;
        item.sourcePathModifierFree = facts.sourcePathModifierFree;
        item.layerIndex = facts.layerIndex;
        item.drawOrder = facts.drawOrder;
        item.fillRule = facts.fillRule;
        item.stroke.width = facts.strokeWidth;
        item.stroke.miterLimit = facts.strokeMiterLimit;
        item.stroke.cap = facts.strokeCap;
        item.stroke.join = facts.strokeJoin;
        item.paint.kind = runtime::PaintKind::Solid;
        item.paint.solid = facts.solid;
        item.localGeometryAvailable = facts.localGeometryAvailable;
        item.localGeometryStaticCandidate = facts.localGeometryStaticCandidate;
        item.localToViewport = *transform;
        item.localPaintAvailable = facts.localPaintAvailable;
        item.localPaintStaticCandidate = facts.localPaintStaticCandidate;
        item.localStroke.width = facts.localStrokeWidth;
        item.localStroke.miterLimit = facts.localStrokeMiterLimit;
        item.localStroke.cap = facts.localStrokeCap;
        item.localStroke.join = facts.localStrokeJoin;
        item.localPaint.kind = runtime::PaintKind::Solid;
        item.localPaint.solid = facts.localSolid;
        item.opacitySeparated = facts.opacitySeparated;
        item.separatedOpacity = facts.separatedOpacity;
        if (!materializeNativeEllipsePath(primitive, nullptr, item.localPath)
            || !materializeNativeEllipsePath(primitive, &*transform, item.path)) {
            return {NativeEllipseFrameCode::UnsupportedNumericOutput,
                    "non-finite ellipse path", std::nullopt};
        }
        scene.statistics.drawItemCount = 1;
        scene.statistics.solidPaintCount = 1;
        scene.statistics.pathVerbCount = item.path.verbs.size();
        scene.statistics.pathPointCount = item.path.points.size();
        scene.controlBounds = item.path.controlBounds;
        scene.drawItems.push_back(std::move(item));
    }
    const auto applied = model::detail::applyAssetModel(certificate_->model, scene);
    if (!applied) {
        return {NativeEllipseFrameCode::ModelApplicationFailed, applied.error, std::nullopt};
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
    return {NativeEllipseFrameCode::Emitted, {}, std::move(scene)};
}

} // namespace avemotion::render::detail
