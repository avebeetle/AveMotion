#include "OwnNativeEllipseStream.hpp"
#include "OwnNativeEllipseStreamCounters.hpp"
#include "OwnVectorModel.hpp"

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

bool precompClipRedundant(const runtime::EvaluatedDrawItem &item,
                          const model::MotionAssetModel &model,
                          const model::MotionSourceNodeRecord &clip,
                          const evaluation::EvaluatedNodeTransform &world, std::size_t width,
                          std::size_t height) {
    if (!world.worldSupported() || world.node != clip.id || clip.layerWidth <= 0 ||
        clip.layerHeight <= 0)
        return false;
    const auto canvas = nativeEllipseViewportTransform(
        world.worldMatrix, model.logicalWidth, model.logicalHeight, width, height);
    if (!canvas || canvas->m12 != 0 || canvas->m21 != 0 || canvas->m11 <= 0 || canvas->m22 <= 0)
        return false;
    // Use the actual float viewport mapping, including its rounded far corners.
    // Exact equality only: a nearly coincident clip can still affect edge pixels.
    const float right = static_cast<float>(clip.layerWidth) * canvas->m11 + canvas->dx;
    const float bottom = static_cast<float>(clip.layerHeight) * canvas->m22 + canvas->dy;
    if (!std::isfinite(right) || !std::isfinite(bottom) || right <= canvas->dx ||
        bottom <= canvas->dy)
        return false;
    if (item.path.points.empty())
        return item.path.verbs.empty();
    const auto &bounds = item.path.controlBounds;
    if (!bounds.valid || !std::isfinite(bounds.left) || !std::isfinite(bounds.top) ||
        !std::isfinite(bounds.right) || !std::isfinite(bounds.bottom) ||
        bounds.left > bounds.right || bounds.top > bounds.bottom)
        return false;
    double extent = 0;
    if (item.stroke.enabled) {
        if (!std::isfinite(item.stroke.width) || item.stroke.width < 0 ||
            !std::isfinite(item.stroke.miterLimit) || item.stroke.miterLimit < 1 ||
            !item.stroke.dashArray.empty())
            return false;
        // Full width exceeds radius*sqrt(2) for square caps. Direct2D's
        // GetMiterLimit contract bounds miter length relative to HALF thickness;
        // the pinned CPU stroker likewise uses radius*miterLimit. The backend
        // uses clipped MITER (not MITER_OR_BEVEL): reserve another radius for
        // the cut face's corner beyond the limited miter centerline. The sum
        // radius*(limit+1) bounds both axial and lateral contributions.
        extent = static_cast<double>(item.stroke.width);
        if (item.stroke.join == runtime::LineJoin::Miter)
            extent = std::max(extent, 0.5 * static_cast<double>(item.stroke.width) *
                                          (static_cast<double>(item.stroke.miterLimit) + 1.0));
    }
    // Cubics lie inside their control hull. For the current solid, undashed
    // Direct2D route reserve two physical pixels beyond its pen hull: one for
    // antialias coverage and one for subpixel curve/edge rasterization. This is
    // a conservative bounded gate, not a promise for arbitrary future backends.
    // Also reserve float-rounding headroom at the actual coordinate magnitude.
    const double magnitude =
        std::max({1.0, std::abs(double(bounds.left)), std::abs(double(bounds.top)),
                  std::abs(double(bounds.right)), std::abs(double(bounds.bottom)),
                  std::abs(double(right)), std::abs(double(bottom))});
    const double margin = 2.0 + 16.0 * std::numeric_limits<float>::epsilon() * magnitude;
    const double padding = extent + margin;
    const double left = double(bounds.left) - padding, top = double(bounds.top) - padding;
    const double farRight = double(bounds.right) + padding,
                 farBottom = double(bounds.bottom) + padding;
    if (!std::isfinite(padding) || !std::isfinite(left) || !std::isfinite(top) ||
        !std::isfinite(farRight) || !std::isfinite(farBottom))
        return false;
    const double intersectLeft = std::max(0.0, left), intersectTop = std::max(0.0, top);
    const double intersectRight = std::min(static_cast<double>(width), farRight),
                 intersectBottom = std::min(static_cast<double>(height), farBottom);
    // An empty target intersection contributes no pixels. Otherwise every side
    // must lie inside this particular mapped clip, with exact float boundaries.
    return intersectLeft >= intersectRight || intersectTop >= intersectBottom ||
           (intersectLeft >= canvas->dx && intersectTop >= canvas->dy &&
            intersectRight <= right && intersectBottom <= bottom);
}

runtime::EvaluatedLayer makeLayer(const model::MotionLayerRecord &source, bool visible,
                                  std::uint32_t drawCount) {
    runtime::EvaluatedLayer layer;
    layer.modelLayer = source.id;
    layer.parentLayer = source.parent.valid() ? source.parent.index() : runtime::kInvalidSceneIndex;
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

std::optional<std::uint64_t> tryNextOwnStreamIdentity(std::atomic<std::uint64_t> &last) noexcept {
    auto current = last.load(std::memory_order_relaxed);
    for (;;) {
        if (current == std::numeric_limits<std::uint64_t>::max())
            return std::nullopt;
        if (last.compare_exchange_weak(current, current + 1U, std::memory_order_relaxed,
                                       std::memory_order_relaxed)) {
            return current + 1U;
        }
    }
}

bool tryAdvanceOwnStreamSequence(std::uint64_t &last) noexcept {
    if (last == std::numeric_limits<std::uint64_t>::max())
        return false;
    ++last;
    return true;
}

OwnNativeEllipseStream::OwnNativeEllipseStream(
    std::shared_ptr<const OwnNativeEllipsePreparedAsset> prepared, std::uint64_t identity)
    : prepared_(std::move(prepared)), identity_(identity),
      evaluator_(prepared_->model, prepared_->vectorAuthored
                     ? std::span<const std::int32_t>(prepared_->vectorAuthored->propertyFrameOffsets)
                     : std::span<const std::int32_t>{}) {}

OwnNativeEllipseStream::~OwnNativeEllipseStream() = default;

OwnNativeEllipseCreateResult
OwnNativeEllipseStream::create(std::shared_ptr<const OwnNativeEllipsePreparedAsset> prepared) {
    if (!prepared || !prepared->model || !prepared->program ||
        bool(prepared->authored) == bool(prepared->vectorAuthored) ||
        prepared->model->totalFrames == 0 || prepared->model->logicalWidth == 0 ||
        prepared->model->logicalHeight == 0 ||
        prepared->program->layers.size() != prepared->model->layers.size() ||
        prepared->program->draws.size() != prepared->model->nodes.size() ||
        prepared->model->geometries.size() != prepared->model->nodes.size() ||
        prepared->model->paints.size() != prepared->model->nodes.size() ||
        std::any_of(
            prepared->model->paints.begin(), prepared->model->paints.end(), [](const auto &paint) {
                return !paint.present || paint.resourceClass == model::ResourceClass::Unknown;
            })) {
        return {OwnNativeEllipseCreateCode::InvalidPreparedAsset,
                "incomplete own ellipse preparation", nullptr};
    }
    const auto source =
        prepared->authored ? prepared->authored->model : prepared->vectorAuthored->model;
    if (!source || prepared->model->sourceAssetHash == 0 ||
        prepared->model->sourceAssetHash != source->sourceAssetHash) {
        return {OwnNativeEllipseCreateCode::InvalidSourceIdentity,
                "invalid own ellipse source identity", nullptr};
    }
    const auto identity = tryNextOwnStreamIdentity(lastOwnStreamIdentity);
    if (!identity) {
        return {OwnNativeEllipseCreateCode::IdentityExhausted, "own stream identities exhausted",
                nullptr};
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

OwnNativeEllipseFrameResult OwnNativeEllipseStream::emit(std::size_t frame, std::size_t width,
                                                         std::size_t height) {
    if (!tryAdvanceOwnStreamSequence(attemptSequence_)) {
        return {OwnNativeEllipseFrameCode::SequenceExhausted,
                "own stream emission sequence exhausted", std::nullopt};
    }
    if (width == 0 || height == 0) {
        return {OwnNativeEllipseFrameCode::InvalidViewport, "zero viewport", std::nullopt};
    }
    const auto &model = *prepared_->model;
    frame = std::min(frame, model.totalFrames - 1U);
    const auto view = evaluator_.evaluate(static_cast<double>(frame), workspace_);
    if (!view) {
        return {OwnNativeEllipseFrameCode::EvaluationFailed, "property evaluation failed",
                std::nullopt};
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
    for (auto child : model.childLayerIds)
        scene.childLayerIndices.push_back(child.index());
    for (const auto &layer : prepared_->program->layers) {
        const bool active = static_cast<double>(frame) >= layer.inFrame &&
                            static_cast<double>(frame) < layer.outFrame;
        auto evaluated = makeLayer(model.layers[layer.layer.index()], active, 0);
        evaluated.firstDrawItem = static_cast<std::uint32_t>(scene.drawItems.size());
        scene.layers.push_back(std::move(evaluated));
        ++scene.statistics.layerCount;
        if (active)
            ++scene.statistics.visibleLayerCount;
        if (!active)
            continue;
        for (std::uint32_t offset = 0; offset < layer.draws.count; ++offset) {
            const auto &binding = prepared_->program->draws[layer.draws.first + offset];
            if (binding.group.index() >= view.nodeTransforms.size()) {
                return {OwnNativeEllipseFrameCode::EvaluationFailed,
                        "bound group transform unavailable", std::nullopt};
            }
            const auto &group = view.nodeTransforms[binding.group.index()];
            if (group.node != binding.group || !group.worldSupported()) {
                return {OwnNativeEllipseFrameCode::EvaluationFailed,
                        "group world transform unavailable", std::nullopt};
            }
            const auto transform = nativeEllipseViewportTransform(
                group.worldMatrix, model.logicalWidth, model.logicalHeight, width, height);
            if (!transform) {
                return {OwnNativeEllipseFrameCode::UnsupportedNumericOutput,
                        "non-finite viewport transform", std::nullopt};
            }
            runtime::EvaluatedDrawItem item;
            const auto &node = model.nodes[binding.node.index()];
            runtime::EvaluatedStroke localStroke;
            runtime::EvaluatedPaint localPaint;
            if (!sampleOwnScenePaint(model, binding, localStroke, localPaint)) {
                return {OwnNativeEllipseFrameCode::EvaluationFailed, "bound paint unavailable",
                        std::nullopt};
            }
            item.modelDrawItem = node.drawItem;
            item.modelNode = node.id;
            item.modelGeometry = node.geometry;
            item.modelPaint = node.paint;
            item.sourcePathNode = binding.path;
            item.sourcePaintNode = binding.paint;
            item.sourcePathCount = 1;
            item.sourcePathModifierFree = !binding.trim;
            item.layerIndex = layer.layer.index();
            item.drawOrder = static_cast<std::uint32_t>(scene.drawItems.size());
            item.fillRule = binding.fillRule;
            item.localGeometryAvailable = true;
            item.localGeometryStaticCandidate =
                binding.geometryClass == model::ResourceClass::AssetStatic;
            item.localToViewport = *transform;
            item.paint = localPaint;
            item.stroke = localStroke;
            item.paint.solid.a = static_cast<std::uint8_t>(static_cast<float>(item.paint.solid.a) *
                                                           group.worldOpacity);
            if (item.stroke.enabled) {
                // Match the existing final-space stroke route. Nonuniform matrices
                // use Telegram's diagonal scale, not a transformed local pen.
                constexpr float sqrt2 = 1.41421F;
                const float dx =
                    sqrt2 * transform->m11 + sqrt2 * transform->m21 + transform->dx - transform->dx;
                const float dy =
                    sqrt2 * transform->m12 + sqrt2 * transform->m22 + transform->dy - transform->dy;
                item.stroke.width *= std::sqrt(dx * dx + dy * dy) / 2.0F;
                if (!std::isfinite(item.stroke.width)) {
                    return {OwnNativeEllipseFrameCode::UnsupportedNumericOutput,
                            "non-finite stroke width", std::nullopt};
                }
            } else {
                item.localPaintAvailable = true;
                item.localPaintStaticCandidate =
                    binding.paintClass == model::ResourceClass::AssetStatic;
                item.localPaint = localPaint;
                item.localStroke = localStroke;
                item.opacitySeparated = true;
                item.separatedOpacity = group.worldOpacity;
            }
            if (!materializeOwnScenePath(model, binding, view, *transform, item.localPath,
                                         item.path)) {
                return {OwnNativeEllipseFrameCode::UnsupportedNumericOutput, "non-finite own path",
                        std::nullopt};
            }
            for (const auto clipId : layer.enclosingClips) {
                const auto* clip = model.sourceNode(clipId);
                if (clip && clipId.index() < view.nodeTransforms.size() &&
                    precompClipRedundant(item, model, *clip, view.nodeTransforms[clipId.index()],
                                         width, height))
                    continue;
                return {
                    OwnNativeEllipseFrameCode::UnsupportedClipping,
                    "precomp clipping required or conservative raster bounds unsupported: frame=" +
                        std::to_string(frame) + " viewport=" + std::to_string(width) + "x" +
                        std::to_string(height) + " node=" + std::to_string(binding.node.index()) +
                        " clip=" + std::to_string(clipId.index()) +
                        " bounds=" + std::to_string(item.path.controlBounds.left) + "," +
                        std::to_string(item.path.controlBounds.top) + "," +
                        std::to_string(item.path.controlBounds.right) + "," +
                        std::to_string(item.path.controlBounds.bottom) +
                        " stroke=" + std::to_string(item.stroke.width) +
                        " miter=" + std::to_string(item.stroke.miterLimit),
                    std::nullopt};
            }
            ++scene.layers.back().drawItemCount;
            ++scene.statistics.drawItemCount;
            ++scene.statistics.solidPaintCount;
            scene.statistics.pathVerbCount += item.path.verbs.size();
            scene.statistics.pathPointCount += item.path.points.size();
            const auto &bounds = item.path.controlBounds;
            if (bounds.valid) {
                if (!scene.controlBounds.valid)
                    scene.controlBounds = bounds;
                else {
                    scene.controlBounds.left = std::min(scene.controlBounds.left, bounds.left);
                    scene.controlBounds.top = std::min(scene.controlBounds.top, bounds.top);
                    scene.controlBounds.right = std::max(scene.controlBounds.right, bounds.right);
                    scene.controlBounds.bottom =
                        std::max(scene.controlBounds.bottom, bounds.bottom);
                }
            }
            scene.drawItems.push_back(std::move(item));
        }
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
