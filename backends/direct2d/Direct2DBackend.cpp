#include "avemotion/backends/direct2d/Direct2DBackend.hpp"
#include "avemotion/model/AssetModel.hpp"
#include "LinearGradientBackendAccess.hpp"
#include "AlphaMatteBackendAccess.hpp"
#include "avemotion/core/Hash.hpp"

#if !defined(_WIN32)
#error "The AveMotion Direct2D backend is available only on Windows."
#endif


#include <d2d1_1.h>
#include <d2d1helper.h>
#include <d2d1effects.h>
#include <wrl/client.h>

#include <bit>
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cmath>
#include <vector>
#include <unordered_map>
#include <utility>

namespace avemotion::backends::direct2d {
namespace {

using Microsoft::WRL::ComPtr;

struct GeometrySlotKey final {
    render::ResourceIdentityScope scope = render::ResourceIdentityScope::Instance;
    std::uint64_t assetHash = 0;
    std::uint64_t assetIdentity = 0;
    std::uint64_t instanceId = 0;
    std::uint64_t instanceIdentity = 0;
    std::uint64_t sourceKey = 0;
    std::uint32_t resourceId = model::kInvalidModelId;

    [[nodiscard]] friend constexpr bool operator==(
        const GeometrySlotKey&,
        const GeometrySlotKey&) noexcept = default;
};

struct GeometrySlotKeyHash final {
    [[nodiscard]] std::size_t operator()(const GeometrySlotKey& key) const noexcept {
        auto value = key.assetHash;
        value ^= key.assetIdentity + 0x9e3779b97f4a7c15ULL
            + (value << 6U) + (value >> 2U);
        value ^= key.instanceId + 0x9e3779b97f4a7c15ULL
            + (value << 6U) + (value >> 2U);
        value ^= key.instanceIdentity + 0x9e3779b97f4a7c15ULL
            + (value << 6U) + (value >> 2U);
        value ^= key.sourceKey + 0x9e3779b97f4a7c15ULL
            + (value << 6U) + (value >> 2U);
        value ^= static_cast<std::uint64_t>(key.resourceId) + 0x9e3779b97f4a7c15ULL
            + (value << 6U) + (value >> 2U);
        value ^= static_cast<std::uint64_t>(key.scope);
        return static_cast<std::size_t>(value);
    }
};

struct GeometryResource final {
    std::uint64_t contentHash = 0;
    std::uint64_t revision = 0;
    ComPtr<ID2D1PathGeometry> geometry;
};

struct StrokeStyleKey final {
    runtime::LineCap cap = runtime::LineCap::Flat;
    runtime::LineJoin join = runtime::LineJoin::Miter;
    std::uint32_t miterBits = 0;

    [[nodiscard]] friend constexpr bool operator==(
        const StrokeStyleKey&,
        const StrokeStyleKey&) noexcept = default;
};

struct StrokeStyleKeyHash final {
    [[nodiscard]] std::size_t operator()(const StrokeStyleKey& key) const noexcept {
        auto value = static_cast<std::uint64_t>(key.miterBits);
        value ^= static_cast<std::uint64_t>(key.cap) << 32U;
        value ^= static_cast<std::uint64_t>(key.join) << 40U;
        return static_cast<std::size_t>(value);
    }
};

[[nodiscard]] GeometrySlotKey slotKey(
    const render::GeometryCacheKey& key) noexcept {
    return {
        .scope = key.scope,
        .assetHash = key.assetHash,
        .assetIdentity = key.assetIdentity,
        .instanceId = key.instanceId,
        .instanceIdentity = key.instanceIdentity,
        .sourceKey = key.sourceKey,
        .resourceId = key.resourceId.value,
    };
}

[[nodiscard]] D2D1::Matrix3x2F toD2D(
    const render::Matrix3x2& value) noexcept {
    return D2D1::Matrix3x2F(
        value.m11,
        value.m12,
        value.m21,
        value.m22,
        value.dx,
        value.dy);
}

class TransformGuard final {
public:
    explicit TransformGuard(ID2D1DeviceContext* context) noexcept
        : context_(context) {
        context_->GetTransform(&original_);
    }

    TransformGuard(const TransformGuard&) = delete;
    TransformGuard& operator=(const TransformGuard&) = delete;

    ~TransformGuard() {
        context_->SetTransform(original_);
    }

    [[nodiscard]] const D2D1::Matrix3x2F& original() const noexcept {
        return original_;
    }

private:
    ID2D1DeviceContext* context_ = nullptr;
    D2D1::Matrix3x2F original_{};
};

[[nodiscard]] D2D1_COLOR_F toD2D(const runtime::Color8& value) noexcept {
    constexpr auto scale = 1.0F / 255.0F;
    return D2D1::ColorF(
        static_cast<float>(value.r) * scale,
        static_cast<float>(value.g) * scale,
        static_cast<float>(value.b) * scale,
        static_cast<float>(value.a) * scale);
}

[[nodiscard]] D2D1_CAP_STYLE mapCap(runtime::LineCap cap) noexcept {
    switch (cap) {
    case runtime::LineCap::Square: return D2D1_CAP_STYLE_SQUARE;
    case runtime::LineCap::Round: return D2D1_CAP_STYLE_ROUND;
    case runtime::LineCap::Flat: return D2D1_CAP_STYLE_FLAT;
    }
    return D2D1_CAP_STYLE_FLAT;
}

[[nodiscard]] D2D1_LINE_JOIN mapJoin(runtime::LineJoin join) noexcept {
    switch (join) {
    case runtime::LineJoin::Bevel: return D2D1_LINE_JOIN_BEVEL;
    case runtime::LineJoin::Round: return D2D1_LINE_JOIN_ROUND;
    case runtime::LineJoin::Miter: return D2D1_LINE_JOIN_MITER;
    }
    return D2D1_LINE_JOIN_MITER;
}

[[nodiscard]] const runtime::CanonicalGeometry* staticGeometry(
    const runtime::EvaluatedScene& scene,
    const runtime::EvaluatedDrawItem& item) noexcept {
    const auto* record = scene.assetModel && item.modelGeometry.valid()
        ? scene.assetModel->geometry(item.modelGeometry)
        : nullptr;
    if (record != nullptr
        && record->resourceClass == model::ResourceClass::AssetStatic
        && record->staticValue) {
        return &*record->staticValue;
    }
    return item.canonicalGeometry.get();
}

[[nodiscard]] const runtime::CanonicalPaint* staticPaint(
    const runtime::EvaluatedScene& scene,
    const runtime::EvaluatedDrawItem& item) noexcept {
    const auto* record = scene.assetModel && item.modelPaint.valid()
        ? scene.assetModel->paint(item.modelPaint)
        : nullptr;
    if (record != nullptr
        && record->resourceClass == model::ResourceClass::AssetStatic
        && record->staticValue) {
        return &*record->staticValue;
    }
    return item.canonicalPaint.get();
}

[[nodiscard]] bool usesLocalRenderingSpace(
    const runtime::EvaluatedScene& scene,
    const runtime::EvaluatedDrawItem& item) noexcept {
    return staticGeometry(scene, item) != nullptr
        || (item.localGeometryAvailable && item.localPaintAvailable);
}

[[nodiscard]] const runtime::EvaluatedPath& selectedPath(
    const runtime::EvaluatedScene& scene,
    const runtime::EvaluatedDrawItem& item) noexcept {
    if (const auto* value = staticGeometry(scene, item)) return value->path;
    return usesLocalRenderingSpace(scene, item) ? item.localPath : item.path;
}

[[nodiscard]] runtime::FillRule selectedFillRule(
    const runtime::EvaluatedScene& scene,
    const runtime::EvaluatedDrawItem& item) noexcept {
    if (const auto* value = staticGeometry(scene, item)) return value->fillRule;
    return item.fillRule;
}

[[nodiscard]] const runtime::EvaluatedStroke& selectedStroke(
    const runtime::EvaluatedScene& scene,
    const runtime::EvaluatedDrawItem& item) noexcept {
    if (const auto* value = staticPaint(scene, item)) return value->stroke;
    return item.localPaintAvailable ? item.localStroke : item.stroke;
}

[[nodiscard]] const runtime::EvaluatedPaint& selectedPaint(
    const runtime::EvaluatedScene& scene,
    const runtime::EvaluatedDrawItem& item) noexcept {
    if (const auto* value = staticPaint(scene, item)) return value->paint;
    return item.localPaintAvailable ? item.localPaint : item.paint;
}

[[nodiscard]] bool finite(float value) noexcept { return std::isfinite(value); }
template <typename Matrix>
[[nodiscard]] bool finiteMatrix(const Matrix& m) noexcept {
    return finite(m.m11) && finite(m.m12) && finite(m.m21) && finite(m.m22)
        && finite(m.dx) && finite(m.dy);
}
template <typename Left, typename Right>
[[nodiscard]] bool sameMatrix(const Left& a, const Right& b) noexcept {
    return a.m11 == b.m11 && a.m12 == b.m12 && a.m21 == b.m21
        && a.m22 == b.m22 && a.dx == b.dx && a.dy == b.dy;
}
[[nodiscard]] bool validPath(const runtime::EvaluatedPath& path) noexcept {
    std::size_t count = 0;
    bool open = false;
    for (auto verb : path.verbs) {
        switch (verb) {
        case runtime::PathVerb::MoveTo: ++count; open = true; break;
        case runtime::PathVerb::LineTo: if (!open) return false; ++count; break;
        case runtime::PathVerb::CubicTo: if (!open) return false; count += 3; break;
        case runtime::PathVerb::Close: if (!open) return false; open = false; break;
        default: return false;
        }
    }
    if (count != path.points.size()) return false;
    for (auto point : path.points) if (!finite(point.x) || !finite(point.y)) return false;
    return true;
}
[[nodiscard]] BackendErrorCode preflight(const render::MotionRenderPlan& plan,
    bool alphaMatteEnabled = false) noexcept {
    using Error = BackendErrorCode;
    const auto& scene = *plan.sourceScene;
    if (!scene.masks.empty()) return Error::UnsupportedFeature;
    for (const auto& layer : scene.layers) {
        if (layer.maskCount != 0 || (!alphaMatteEnabled && layer.matte != runtime::MatteMode::None)
            || !layer.clipPath.verbs.empty() || !layer.clipPath.points.empty())
            return Error::UnsupportedFeature;
    }
    for (const auto& item : plan.drawItems) {
        if (item.sourceDrawItemIndex >= scene.drawItems.size()) return Error::InvalidPlan;
        const auto& source = scene.drawItems[item.sourceDrawItemIndex];
        if (source.layerIndex >= scene.layers.size()
            || !finiteMatrix(item.geometryTransform) || !finiteMatrix(item.presentationTransform)
            || !finite(item.effectiveOpacity) || item.effectiveOpacity < 0 || item.effectiveOpacity > 1
            || (source.localGeometryAvailable != source.localPaintAvailable &&
                !(alphaMatteEnabled && source.localGeometryAvailable && !source.localPaintAvailable
                  && source.stroke.enabled && !source.canonicalGeometry && !source.canonicalPaint)))
            return Error::InvalidPlan;
        // The executor's frozen-model selection must be precisely the alias
        // which the planner selected. A missing alias cannot repair a plan.
        if (scene.assetModel && source.modelGeometry.valid()) {
            const auto* record = scene.assetModel->geometry(source.modelGeometry);
            if (!record || (record->resourceClass == model::ResourceClass::AssetStatic
                && record->staticValue && source.canonicalGeometry.get() != &*record->staticValue))
                return Error::InvalidPlan;
        }
        if (scene.assetModel && source.modelPaint.valid()) {
            const auto* record = scene.assetModel->paint(source.modelPaint);
            if (!record || (record->resourceClass == model::ResourceClass::AssetStatic
                && record->staticValue && source.canonicalPaint.get() != &*record->staticValue))
                return Error::InvalidPlan;
        }
        if ((source.canonicalGeometry || source.canonicalPaint)
            && !(source.localGeometryAvailable && source.localPaintAvailable)) return Error::InvalidPlan;
        const auto local = usesLocalRenderingSpace(scene, source);
        if ((local && (!finiteMatrix(source.localToViewport)
                      || !sameMatrix(item.geometryTransform, source.localToViewport)))
            || (!local && !sameMatrix(item.geometryTransform, render::Matrix3x2::identity())))
            return Error::InvalidPlan;
        const auto& paint = selectedPaint(scene, source);
        const auto& stroke = selectedStroke(scene, source);
        if (!validPath(selectedPath(scene, source))
            || (selectedFillRule(scene, source) != runtime::FillRule::EvenOdd
                && selectedFillRule(scene, source) != runtime::FillRule::Winding)
            || !finite(stroke.width) || stroke.width < 0 || !finite(stroke.miterLimit)
            || stroke.miterLimit < 0
            || (stroke.cap != runtime::LineCap::Flat && stroke.cap != runtime::LineCap::Square
                && stroke.cap != runtime::LineCap::Round)
            || (stroke.join != runtime::LineJoin::Miter && stroke.join != runtime::LineJoin::Bevel
                && stroke.join != runtime::LineJoin::Round)) return Error::InvalidPlan;
        if (!stroke.dashArray.empty()) return Error::UnsupportedFeature;
        std::uint32_t bits = stroke.enabled ? render::RenderFeatureStroke : 0U;
        if (paint.kind == runtime::PaintKind::Solid) bits |= render::RenderFeatureSolidPaint;
        else if (paint.kind == runtime::PaintKind::Gradient) bits |= render::RenderFeatureGradientPaint;
        else return Error::UnsupportedFeature;
        if (alphaMatteEnabled) {
            const auto& layer = scene.layers[source.layerIndex];
            if (layer.matteSourceLayer != runtime::kInvalidSceneIndex ||
                layer.matteTargetLayer != runtime::kInvalidSceneIndex) bits |= render::RenderFeatureMatte;
            if (layer.isolatedBottomDifference) bits |= render::RenderFeatureBlend;
        }
        if (bits != item.featureBits) return Error::UnsupportedFeature;
        if (paint.kind == runtime::PaintKind::Gradient) {
            if (stroke.enabled || paint.gradient.kind != runtime::GradientKind::Linear)
                return Error::UnsupportedFeature;
            if (!local || !source.localGeometryAvailable || !source.localPaintAvailable)
                return Error::InvalidPlan;
            const auto& g = paint.gradient;
            if (!finite(g.start.x) || !finite(g.start.y) || !finite(g.end.x) || !finite(g.end.y)
                || (g.start.x == g.end.x && g.start.y == g.end.y)
                || g.stops.size() < 2 || g.stops.size() > 16
                || g.center.x != 0 || g.center.y != 0 || g.focal.x != 0 || g.focal.y != 0
                || g.centerRadius != 0 || g.focalRadius != 0
                || paint.solid.r != 0 || paint.solid.g != 0 || paint.solid.b != 0 || paint.solid.a != 0
                || paint.image.present || paint.image.width != 0 || paint.image.height != 0)
                return Error::UnsupportedFeature;
            for (const auto value : paint.image.matrix) if (value != 0) return Error::UnsupportedFeature;
            float previous = -1;
            const auto alpha = g.stops.front().color.a;
            for (const auto& stop : g.stops) {
                if (!finite(stop.position) || stop.position < 0 || stop.position > 1
                    || stop.position <= previous || stop.color.a != alpha) return Error::UnsupportedFeature;
                previous = stop.position;
            }
        }
    }
    return Error::None;
}

[[nodiscard]] bool validRect(const runtime::RectF& r) noexcept {
    return !r.valid || (finite(r.left) && finite(r.top) && finite(r.right) && finite(r.bottom)
        && r.left <= r.right && r.top <= r.bottom);
}

// Match the planner's resource payload schema without consulting mutable caches.
[[nodiscard]] std::uint64_t geometryHash(const runtime::EvaluatedScene& scene,
    const runtime::EvaluatedDrawItem& source) noexcept {
    if (source.canonicalGeometry) return source.canonicalGeometry->contentHash;
    const auto& path = selectedPath(scene, source);
    core::Fnv1a64 hash;
    hash.appendU8(static_cast<std::uint8_t>(selectedFillRule(scene, source)));
    hash.appendU64(path.verbs.size()); hash.appendU64(path.points.size());
    for (auto verb : path.verbs) hash.appendU8(static_cast<std::uint8_t>(verb));
    for (auto point : path.points) { hash.appendFloat(point.x); hash.appendFloat(point.y); }
    hash.appendU8(path.controlBounds.valid ? 1U : 0U);
    if (path.controlBounds.valid) {
        hash.appendFloat(path.controlBounds.left); hash.appendFloat(path.controlBounds.top);
        hash.appendFloat(path.controlBounds.right); hash.appendFloat(path.controlBounds.bottom);
    }
    return hash.value();
}
[[nodiscard]] std::uint64_t paintHash(const runtime::EvaluatedScene& scene,
    const runtime::EvaluatedDrawItem& source) noexcept {
    if (source.canonicalPaint) return source.canonicalPaint->contentHash;
    const auto& paint = selectedPaint(scene, source);
    const auto& stroke = selectedStroke(scene, source);
    core::Fnv1a64 hash;
    const auto color = [&](const runtime::Color8& c) {
        hash.appendU8(c.r); hash.appendU8(c.g); hash.appendU8(c.b); hash.appendU8(c.a);
    };
    hash.appendU8(static_cast<std::uint8_t>(paint.kind)); hash.appendU8(stroke.enabled ? 1U : 0U);
    hash.appendFloat(stroke.width); hash.appendFloat(stroke.miterLimit);
    hash.appendU8(static_cast<std::uint8_t>(stroke.cap)); hash.appendU8(static_cast<std::uint8_t>(stroke.join));
    hash.appendU64(stroke.dashArray.size()); for (auto dash : stroke.dashArray) hash.appendFloat(dash);
    if (paint.kind == runtime::PaintKind::Solid) color(paint.solid);
    else if (paint.kind == runtime::PaintKind::Gradient) {
        const auto& g = paint.gradient;
        hash.appendU8(static_cast<std::uint8_t>(g.kind));
        for (auto value : {g.start.x,g.start.y,g.end.x,g.end.y,g.center.x,g.center.y,
                          g.focal.x,g.focal.y,g.centerRadius,g.focalRadius}) hash.appendFloat(value);
        hash.appendU64(g.stops.size());
        for (const auto& stop : g.stops) { hash.appendFloat(stop.position); color(stop.color); }
    }
    return hash.value();
}

[[nodiscard]] BackendErrorCode preflightAlpha(const render::MotionRenderPlan& plan) {
    using Error = BackendErrorCode;
    constexpr auto invalid = runtime::kInvalidSceneIndex;
    const auto& scene = *plan.sourceScene;
    if (!finite(plan.presentationOpacity) || plan.presentationOpacity < 0 || plan.presentationOpacity > 1
        || !validRect(plan.presentedBounds) || !validRect(plan.dirtyRegion)
        || plan.stamp.assetHash != scene.sourceAssetHash || plan.stamp.instanceId != scene.instanceId
        || plan.stamp.assetIdentity != (scene.assetHandle.valid() ? scene.assetHandle.packed() : 0)
        || plan.stamp.instanceIdentity != (scene.instanceHandle.valid() ? scene.instanceHandle.packed() : scene.instanceId)
        || plan.stamp.evaluationSequence != scene.evaluationSequence) return Error::InvalidPlan;
    unsigned pairs = 0;
    if (scene.assetModel && (!scene.assetModelApplied || scene.assetModel->sourceAssetHash != scene.sourceAssetHash))
        return Error::InvalidPlan;
    for (std::size_t i = 0; i < scene.layers.size(); ++i) {
        const auto& layer = scene.layers[i];
        const auto source = layer.matteSourceLayer, target = layer.matteTargetLayer;
        if (scene.assetModel) {
            const auto* frozen = scene.assetModel->layer(layer.modelLayer);
            if (!frozen || frozen->id != layer.modelLayer || frozen->matte != layer.matte
                || (frozen->parent.valid() ? frozen->parent.index() : invalid) != layer.parentLayer
                || ((source != invalid || target != invalid) &&
                    (frozen->nodes.count != 1 || !(frozen->dependencyBits & model::StaticDependencyMatte))))
                return Error::InvalidPlan;
        }
        if (!finite(layer.opacity) || layer.opacity < 0 || layer.opacity > 1
            || layer.firstDrawItem > scene.drawItems.size()
            || layer.drawItemCount > scene.drawItems.size() - layer.firstDrawItem
            || (layer.parentLayer != invalid && (layer.parentLayer >= scene.layers.size() || layer.parentLayer == i)))
            return Error::InvalidPlan;
        for (unsigned n = 0; n < layer.drawItemCount; ++n)
            if (scene.drawItems[layer.firstDrawItem+n].layerIndex != i) return Error::InvalidPlan;
        if (source != invalid || target != invalid) {
            if (source != invalid && target != invalid) return Error::InvalidPlan;
            if (layer.parentLayer != 0 || scene.layers.empty() || scene.layers[0].parentLayer != invalid
                || layer.drawItemCount > 1 || layer.isolatedBottomDifference) return Error::InvalidPlan;
            if (source != invalid) {
                if (source >= scene.layers.size() || source != i+1 || layer.matte != runtime::MatteMode::Alpha)
                    return Error::InvalidPlan;
                const auto& other = scene.layers[source];
                if (other.matteTargetLayer != i || other.matteSourceLayer != invalid || other.matte != runtime::MatteMode::None)
                    return Error::InvalidPlan;
                if (++pairs > 64) return Error::InvalidPlan;
            } else {
                if (target >= scene.layers.size() || i != static_cast<std::size_t>(target)+1
                    || layer.matte != runtime::MatteMode::None || scene.layers[target].matteSourceLayer != i)
                    return Error::InvalidPlan;
            }
        } else if (layer.matte != runtime::MatteMode::None) return Error::UnsupportedFeature;
        if (layer.isolatedBottomDifference && (i != 1 || layer.parentLayer != 0
            || scene.layers[0].parentLayer != invalid || layer.matte != runtime::MatteMode::None))
            return Error::UnsupportedFeature;
    }
    // Validate even invisible source payloads, then require the entire eligible
    // source sequence exactly once. A valid prefix cannot issue native work.
    render::MotionRenderPlan validation;
    validation.sourceScene = plan.sourceScene;
    validation.drawItems.reserve(scene.drawItems.size());
    std::size_t next = 0;
    for (std::size_t i = 0; i < scene.drawItems.size(); ++i) {
        const auto& source = scene.drawItems[i];
        if (source.layerIndex >= scene.layers.size()) return Error::InvalidPlan;
        const auto& layer = scene.layers[source.layerIndex];
        const auto validOrigin = [](runtime::EvaluatedValueOrigin origin) {
            return origin == runtime::EvaluatedValueOrigin::Unknown || origin == runtime::EvaluatedValueOrigin::AssetStatic
                || origin == runtime::EvaluatedValueOrigin::InstanceEvaluated;
        };
        if (!validOrigin(source.geometryOrigin) || !validOrigin(source.paintOrigin)) return Error::InvalidPlan;
        if (!scene.assetModel && (source.canonicalGeometry || source.canonicalPaint || source.modelNode.valid()
            || source.modelGeometry.valid() || source.modelPaint.valid() || source.modelDrawItem.valid())) return Error::InvalidPlan;
        if (scene.assetModel) {
            const auto* node = scene.assetModel->node(source.modelNode);
            const auto* geometry = scene.assetModel->geometry(source.modelGeometry);
            const auto* paintRecord = scene.assetModel->paint(source.modelPaint);
            if (!node || !geometry || !paintRecord || node->id != source.modelNode
                || node->layer != layer.modelLayer || node->drawItem != source.modelDrawItem
                || node->geometry != source.modelGeometry || node->paint != source.modelPaint
                || geometry->id != source.modelGeometry || paintRecord->id != source.modelPaint
                || source.sourceGeometryId != std::uint64_t(source.modelGeometry.value)+1
                || source.sourcePaintId != std::uint64_t(source.modelPaint.value)+1) return Error::InvalidPlan;
            // Supplied canonical payloads must be the frozen static value,
            // never an instance/unknown alias carrying a trusted-looking hash.
            const auto validAlias = [](const auto& record, const auto& alias) {
                return !alias || (record.resourceClass == model::ResourceClass::AssetStatic
                    && record.staticValue && alias.get() == &*record.staticValue);
            };
            if (!validAlias(*geometry, source.canonicalGeometry)
                || !validAlias(*paintRecord, source.canonicalPaint)) return Error::InvalidPlan;
        }
        if (i < layer.firstDrawItem || i-layer.firstDrawItem >= layer.drawItemCount
            || !finite(source.separatedOpacity) || source.separatedOpacity < 0 || source.separatedOpacity > 1
            || !validRect(selectedPath(scene,source).controlBounds)) return Error::InvalidPlan;
        const auto inherited = source.opacitySeparated ? source.separatedOpacity : layer.opacity;
        const auto opacity = (layer.matteTargetLayer != invalid ? 1.F : plan.presentationOpacity) * inherited;
        const auto& paint = selectedPaint(scene,source);
        const auto& stroke = selectedStroke(scene,source);
        std::uint32_t bits = paint.kind == runtime::PaintKind::Solid ? render::RenderFeatureSolidPaint
            : paint.kind == runtime::PaintKind::Gradient ? render::RenderFeatureGradientPaint : 0U;
        if (stroke.enabled) bits |= render::RenderFeatureStroke;
        if (layer.matteSourceLayer != invalid || layer.matteTargetLayer != invalid) bits |= render::RenderFeatureMatte;
        if (layer.isolatedBottomDifference) bits |= render::RenderFeatureBlend;
        render::MotionDrawItem checked;
        checked.sourceDrawItemIndex = static_cast<std::uint32_t>(i);
        checked.geometryTransform = usesLocalRenderingSpace(scene,source)
            ? render::Matrix3x2{source.localToViewport.m11,source.localToViewport.m12,source.localToViewport.m21,
                source.localToViewport.m22,source.localToViewport.dx,source.localToViewport.dy}
            : render::Matrix3x2::identity();
        checked.featureBits = bits;
        if (plan.presentationVisible && plan.presentationOpacity > 0 && layer.visible && opacity > 0) {
            if (next >= plan.drawItems.size()) return Error::InvalidPlan;
            checked = plan.drawItems[next];
            if (checked.sourceDrawItemIndex != i || checked.drawOrder != next || checked.effectiveOpacity != opacity
                || checked.featureBits != bits || !validRect(checked.presentedBounds)) return Error::InvalidPlan;
            core::Fnv1a64 fallback;
            fallback.appendU64(scene.sourceAssetHash); fallback.appendU32(source.layerIndex);
            fallback.appendU32(static_cast<std::uint32_t>(i-layer.firstDrawItem));
            const auto key = fallback.value();
            const bool aware = scene.assetModelApplied;
            if (checked.sourceItemKey != (aware && source.modelNode.valid() ? std::uint64_t(source.modelNode.value)+1 : key)
                || checked.drawItem != (aware ? source.modelDrawItem : model::DrawItemId{})
                || checked.node != (aware ? source.modelNode : model::NodeId{})) return Error::InvalidPlan;
            const auto validKey = [&](const auto& resource, auto id, std::uint64_t sourceKey,
                                      std::uint64_t hash, runtime::EvaluatedValueOrigin origin) {
                const auto asset = origin == runtime::EvaluatedValueOrigin::AssetStatic;
                return resource.scope == (asset ? render::ResourceIdentityScope::Asset : render::ResourceIdentityScope::Instance)
                    && resource.assetHash == plan.stamp.assetHash && resource.assetIdentity == plan.stamp.assetIdentity
                    && resource.instanceId == (asset ? 0 : plan.stamp.instanceId)
                    && resource.instanceIdentity == (asset ? 0 : plan.stamp.instanceIdentity)
                    && resource.sourceKey == (sourceKey ? sourceKey : key) && resource.resourceId == id
                    && resource.contentHash == hash && (!asset || resource.revision == 0);
            };
            if (!validKey(checked.geometry, aware && !source.sourceGeometryProjected ? source.modelGeometry : model::GeometryId{},
                          source.sourceGeometryId,geometryHash(scene,source),source.geometryOrigin)
                || !validKey(checked.paint, aware ? source.modelPaint : model::PaintId{},
                          source.sourcePaintId,paintHash(scene,source),source.paintOrigin)
                || (source.sourceGeometryRevisionAuthoritative && checked.geometry.scope == render::ResourceIdentityScope::Instance
                    && checked.geometry.revision != source.sourceGeometryRevision)) return Error::InvalidPlan;
            if (next && !sameMatrix(checked.presentationTransform,plan.drawItems[0].presentationTransform))
                return Error::InvalidPlan;
            ++next;
        }
        validation.drawItems.push_back(checked);
    }
    if (next != plan.drawItems.size()) return Error::InvalidPlan;
    return preflight(validation,true);
}

struct GradientResource final {
    std::uint64_t contentHash = 0;
    std::uint64_t revision = 0;
    ComPtr<ID2D1GradientStopCollection> stops;
    ComPtr<ID2D1LinearGradientBrush> brush;
};
[[nodiscard]] GeometrySlotKey slotKey(const render::PaintCacheKey& key) noexcept {
    return {key.scope, key.assetHash, key.assetIdentity, key.instanceId,
        key.instanceIdentity, key.sourceKey, key.resourceId.value};
}

[[nodiscard]] HRESULT buildPathGeometry(
    ID2D1Factory* factory,
    const runtime::EvaluatedScene& scene,
    const runtime::EvaluatedDrawItem& item,
    ID2D1PathGeometry** output) noexcept {
    if (factory == nullptr || output == nullptr) {
        return E_INVALIDARG;
    }
    *output = nullptr;
    ComPtr<ID2D1PathGeometry> geometry;
    auto hr = factory->CreatePathGeometry(&geometry);
    if (FAILED(hr)) {
        return hr;
    }
    ComPtr<ID2D1GeometrySink> sink;
    hr = geometry->Open(&sink);
    if (FAILED(hr)) {
        return hr;
    }
    const auto& path = selectedPath(scene, item);
    sink->SetFillMode(selectedFillRule(scene, item) == runtime::FillRule::EvenOdd
        ? D2D1_FILL_MODE_ALTERNATE
        : D2D1_FILL_MODE_WINDING);

    std::size_t pointIndex = 0;
    bool figureOpen = false;
    for (const auto verb : path.verbs) {
        switch (verb) {
        case runtime::PathVerb::MoveTo:
            if (pointIndex >= path.points.size()) {
                return E_INVALIDARG;
            }
            if (figureOpen) {
                sink->EndFigure(D2D1_FIGURE_END_OPEN);
            }
            sink->BeginFigure(
                D2D1::Point2F(
                    path.points[pointIndex].x,
                    path.points[pointIndex].y),
                D2D1_FIGURE_BEGIN_FILLED);
            ++pointIndex;
            figureOpen = true;
            break;
        case runtime::PathVerb::LineTo:
            if (!figureOpen || pointIndex >= path.points.size()) {
                return E_INVALIDARG;
            }
            sink->AddLine(D2D1::Point2F(
                path.points[pointIndex].x,
                path.points[pointIndex].y));
            ++pointIndex;
            break;
        case runtime::PathVerb::CubicTo:
            if (!figureOpen || pointIndex + 2U >= path.points.size()) {
                return E_INVALIDARG;
            }
            sink->AddBezier(D2D1::BezierSegment(
                D2D1::Point2F(
                    path.points[pointIndex].x,
                    path.points[pointIndex].y),
                D2D1::Point2F(
                    path.points[pointIndex + 1U].x,
                    path.points[pointIndex + 1U].y),
                D2D1::Point2F(
                    path.points[pointIndex + 2U].x,
                    path.points[pointIndex + 2U].y)));
            pointIndex += 3U;
            break;
        case runtime::PathVerb::Close:
            if (!figureOpen) {
                return E_INVALIDARG;
            }
            sink->EndFigure(D2D1_FIGURE_END_CLOSED);
            figureOpen = false;
            break;
        }
    }
    if (figureOpen) {
        sink->EndFigure(D2D1_FIGURE_END_OPEN);
    }
    if (pointIndex != path.points.size()) {
        return E_INVALIDARG;
    }
    hr = sink->Close();
    if (FAILED(hr)) {
        return hr;
    }
    *output = geometry.Detach();
    return S_OK;
}

} // namespace

struct Backend::Impl final {
    std::uint64_t graphicsDomainId = 0;
    std::uint64_t graphicsGeneration = 0;
    ID2D1Factory* factoryIdentity = nullptr;
    ComPtr<ID2D1Factory> factory;
    ID2D1DeviceContext* brushContextIdentity = nullptr;
    ComPtr<ID2D1SolidColorBrush> solidBrush;
    std::unordered_map<
        GeometrySlotKey,
        GeometryResource,
        GeometrySlotKeyHash> geometryCache;
    std::unordered_map<
        StrokeStyleKey,
        ComPtr<ID2D1StrokeStyle>,
        StrokeStyleKeyHash> strokeStyleCache;
    BackendDiagnostics diagnostics;
    std::unordered_map<GeometrySlotKey, GradientResource, GeometrySlotKeyHash> gradientCache;

    void resetResources() noexcept {
        geometryCache.clear();
        strokeStyleCache.clear();
        gradientCache.clear();
        solidBrush.Reset();
        brushContextIdentity = nullptr;
        factory.Reset();
        factoryIdentity = nullptr;
        ++diagnostics.resourceDomainResets;
    }

    [[nodiscard]] HRESULT ensureDomain(const RenderSession& session) noexcept {
        const auto domainChanged = graphicsDomainId != session.graphicsDomainId
            || graphicsGeneration != session.graphicsGeneration;
        if (domainChanged) {
            resetResources();
            graphicsDomainId = session.graphicsDomainId;
            graphicsGeneration = session.graphicsGeneration;
        }
        ComPtr<ID2D1Factory> currentFactory;
        session.deviceContext->GetFactory(&currentFactory);
        if (!currentFactory) {
            return E_FAIL;
        }
        if (factoryIdentity != currentFactory.Get()) {
            // A graphics-domain transition already cleared the resources.
            // Avoid counting and performing a second reset merely because the
            // new factory is being adopted for the first time.
            if (!domainChanged && factoryIdentity != nullptr) {
                resetResources();
            }
            graphicsDomainId = session.graphicsDomainId;
            graphicsGeneration = session.graphicsGeneration;
            factory = std::move(currentFactory);
            factoryIdentity = factory.Get();
        }
        if (brushContextIdentity != session.deviceContext || !solidBrush) {
            gradientCache.clear();
            solidBrush.Reset();
            const auto hr = session.deviceContext->CreateSolidColorBrush(
                D2D1::ColorF(D2D1::ColorF::White),
                &solidBrush);
            if (FAILED(hr)) {
                return hr;
            }
            brushContextIdentity = session.deviceContext;
        }
        return S_OK;
    }
};

Backend::Backend()
    : impl_(std::make_unique<Impl>()) {
}

Backend::Backend(Backend&&) noexcept = default;
Backend& Backend::operator=(Backend&&) noexcept = default;
Backend::~Backend() = default;

DrawResult Backend::draw(
    const render::MotionRenderPlan& plan,
    const RenderSession& session) {
    return execute(plan, session, false);
}

DrawResult Backend::drawFixedOwnProfile(
    const render::MotionRenderPlan& plan,
    const RenderSession& session) {
    return detail::AlphaMatteBackendAccess::draw(*this, plan, session);
}

DrawResult detail::LinearGradientBackendAccess::draw(Backend& backend,
    const render::MotionRenderPlan& plan, const RenderSession& session) {
    return backend.execute(plan, session, true);
}

DrawResult detail::AlphaMatteBackendAccess::draw(Backend& backend,
    const render::MotionRenderPlan& plan, const RenderSession& session) {
    return backend.execute(plan, session, true, true);
}

DrawResult Backend::execute(const render::MotionRenderPlan& plan,
    const RenderSession& session, bool linearFillEnabled, bool alphaMatteEnabled) {
    DrawResult result;
    if (session.deviceContext == nullptr || !plan.sourceScene) {
        result.error = {
            BackendErrorCode::InvalidArgument,
            static_cast<long>(E_INVALIDARG),
            "a valid device context and render plan source scene are required"};
        return result;
    }
    if (linearFillEnabled) {
        const auto error = alphaMatteEnabled ? preflightAlpha(plan) : preflight(plan);
        if (error != BackendErrorCode::None) {
            result.error = {error, static_cast<long>(E_INVALIDARG),
                "private Direct2D plan failed whole-plan preflight"};
            return result;
        }
    }
    const auto domainResult = impl_->ensureDomain(session);
    if (FAILED(domainResult)) {
        result.error = {
            domainResult == D2DERR_RECREATE_TARGET
                ? BackendErrorCode::DeviceLost
                : BackendErrorCode::ResourceCreationFailed,
            static_cast<long>(domainResult),
            "unable to initialize the Direct2D resource domain"};
        return result;
    }

    const TransformGuard transformGuard(session.deviceContext);
    ++impl_->diagnostics.drawCalls;

    const auto issuePrimitive = [&](const render::MotionDrawItem& planItem) -> DrawResult {
        ++impl_->diagnostics.drawItemsVisited;
        if (planItem.sourceDrawItemIndex >= plan.sourceScene->drawItems.size()) {
            result.error = {
                BackendErrorCode::InvalidPlan,
                static_cast<long>(E_INVALIDARG),
                "draw item references an invalid source item"};
            return result;
        }
        const auto& source = plan.sourceScene->drawItems[planItem.sourceDrawItemIndex];
        const auto& paint = selectedPaint(*plan.sourceScene, source);
        const auto& stroke = selectedStroke(*plan.sourceScene, source);
        const auto unsupported = planItem.featureBits
            & ~(render::RenderFeatureSolidPaint | render::RenderFeatureStroke);
        if (!linearFillEnabled && (unsupported != 0U || paint.kind != runtime::PaintKind::Solid)) {
            ++result.itemsSkipped;
            ++impl_->diagnostics.unsupportedItemsSkipped;
            return result;
        }

        const auto nativeSlot = slotKey(planItem.geometry);
        auto found = impl_->geometryCache.find(nativeSlot);
        ComPtr<ID2D1PathGeometry> geometry;
        if (found != impl_->geometryCache.end()
            && found->second.contentHash == planItem.geometry.contentHash
            && found->second.revision == planItem.geometry.revision) {
            geometry = found->second.geometry;
            ++impl_->diagnostics.geometryCacheHits;
        } else {
            ++impl_->diagnostics.geometryCacheMisses;
            const auto hr = buildPathGeometry(
                impl_->factory.Get(), *plan.sourceScene, source, &geometry);
            if (FAILED(hr)) {
                result.error = {
                    BackendErrorCode::ResourceCreationFailed,
                    static_cast<long>(hr),
                    "unable to create a Direct2D path geometry"};
                return result;
            }
            GeometryResource resource{
                .contentHash = planItem.geometry.contentHash,
                .revision = planItem.geometry.revision,
                .geometry = geometry,
            };
            if (found == impl_->geometryCache.end()) {
                impl_->geometryCache.emplace(nativeSlot, std::move(resource));
                ++impl_->diagnostics.geometryResourcesCreated;
            } else {
                found->second = std::move(resource);
                ++impl_->diagnostics.geometryResourcesReplaced;
            }
        }

        ID2D1Brush* brush = impl_->solidBrush.Get();
        if (paint.kind == runtime::PaintKind::Gradient) {
            const auto key = slotKey(planItem.paint);
            auto gradient = impl_->gradientCache.find(key);
            if (gradient == impl_->gradientCache.end()
                || gradient->second.contentHash != planItem.paint.contentHash
                || gradient->second.revision != planItem.paint.revision) {
                GradientResource resource;
                resource.contentHash = planItem.paint.contentHash;
                resource.revision = planItem.paint.revision;
                std::vector<D2D1_GRADIENT_STOP> stops;
                stops.reserve(paint.gradient.stops.size());
                for (const auto& stop : paint.gradient.stops) stops.push_back({stop.position, toD2D(stop.color)});
                auto hr = session.deviceContext->CreateGradientStopCollection(stops.data(),
                    static_cast<UINT32>(stops.size()), D2D1_GAMMA_2_2, D2D1_EXTEND_MODE_CLAMP, &resource.stops);
                if (SUCCEEDED(hr)) {
                    const D2D1_LINEAR_GRADIENT_BRUSH_PROPERTIES properties{
                        D2D1::Point2F(paint.gradient.start.x, paint.gradient.start.y),
                        D2D1::Point2F(paint.gradient.end.x, paint.gradient.end.y)};
                    hr = session.deviceContext->CreateLinearGradientBrush(properties, resource.stops.Get(), &resource.brush);
                }
                if (FAILED(hr)) {
                    result.error = {hr == D2DERR_RECREATE_TARGET ? BackendErrorCode::DeviceLost
                        : BackendErrorCode::ResourceCreationFailed, static_cast<long>(hr),
                        "unable to create a Direct2D linear gradient"};
                    return result;
                }
                gradient = impl_->gradientCache.insert_or_assign(key, std::move(resource)).first;
            }
            brush = gradient->second.brush.Get();
        } else {
            impl_->solidBrush->SetColor(toD2D(paint.solid));
        }
        brush->SetOpacity(planItem.effectiveOpacity);
        session.deviceContext->SetTransform(
            toD2D(planItem.geometryTransform)
            * toD2D(planItem.presentationTransform)
            * transformGuard.original());
        if (stroke.enabled) {
            const StrokeStyleKey styleKey{
                .cap = stroke.cap,
                .join = stroke.join,
                .miterBits = std::bit_cast<std::uint32_t>(stroke.miterLimit),
            };
            ComPtr<ID2D1StrokeStyle> strokeStyle;
            const auto styleFound = impl_->strokeStyleCache.find(styleKey);
            if (styleFound != impl_->strokeStyleCache.end()) {
                strokeStyle = styleFound->second;
                ++impl_->diagnostics.strokeStyleCacheHits;
            } else {
                ++impl_->diagnostics.strokeStyleCacheMisses;
                const auto properties = D2D1::StrokeStyleProperties(
                    mapCap(stroke.cap),
                    mapCap(stroke.cap),
                    mapCap(stroke.cap),
                    mapJoin(stroke.join),
                    stroke.miterLimit,
                    D2D1_DASH_STYLE_SOLID,
                    0.0F);
                const auto hr = impl_->factory->CreateStrokeStyle(
                    properties,
                    nullptr,
                    0U,
                    &strokeStyle);
                if (FAILED(hr)) {
                    result.error = {
                        BackendErrorCode::ResourceCreationFailed,
                        static_cast<long>(hr),
                        "unable to create a Direct2D stroke style"};
                    return result;
                }
                impl_->strokeStyleCache.emplace(styleKey, strokeStyle);
                ++impl_->diagnostics.strokeStylesCreated;
            }
            session.deviceContext->DrawGeometry(
                geometry.Get(),
                brush,
                stroke.width,
                strokeStyle.Get());
        } else {
            session.deviceContext->FillGeometry(
                geometry.Get(),
                brush);
        }
        ++result.itemsDrawn;
        if (paint.kind == runtime::PaintKind::Solid) ++impl_->diagnostics.solidItemsDrawn;
        return result;
    };
    const bool isolate = alphaMatteEnabled && std::any_of(plan.sourceScene->layers.begin(),
        plan.sourceScene->layers.end(), [](const auto& layer) {
            return layer.matteSourceLayer != runtime::kInvalidSceneIndex || layer.isolatedBottomDifference;
        });
    if (!isolate) {
        for (const auto& item : plan.drawItems) if (!issuePrimitive(item)) return result;
        return result;
    }

    auto* context = session.deviceContext;
    struct TargetGuard final {
        ID2D1DeviceContext* context;
        ComPtr<ID2D1Image> original;
        explicit TargetGuard(ID2D1DeviceContext* value) : context(value) { context->GetTarget(&original); }
        ~TargetGuard() { context->SetTarget(original.Get()); }
    } targetGuard(context);
    const auto failure = [&](HRESULT hr, const char* message) {
        result.error = {hr == D2DERR_RECREATE_TARGET ? BackendErrorCode::DeviceLost
            : BackendErrorCode::ResourceCreationFailed, static_cast<long>(hr), message};
        return result;
    };
    ComPtr<ID2D1CommandList> frame;
    auto hr = context->CreateCommandList(&frame);
    if (FAILED(hr)) return failure(hr,"unable to create isolated frame command list");
    context->SetTarget(frame.Get());
    std::vector<bool> consumed(plan.sourceScene->layers.size(),false);
    for (const auto& item : plan.drawItems) {
        const auto& source = plan.sourceScene->drawItems[item.sourceDrawItemIndex];
        const auto& layer = plan.sourceScene->layers[source.layerIndex];
        const bool contentRole = layer.matteSourceLayer != runtime::kInvalidSceneIndex;
        const bool providerRole = layer.matteTargetLayer != runtime::kInvalidSceneIndex;
        if (!contentRole && !providerRole) {
            if (!issuePrimitive(item)) return result;
            continue;
        }
        const auto contentIndex = contentRole ? source.layerIndex : layer.matteTargetLayer;
        const auto providerIndex = plan.sourceScene->layers[contentIndex].matteSourceLayer;
        if (consumed[contentIndex]) continue;
        consumed[contentIndex] = true;
        // Each pair owns its effect and child recordings. Effects retain input
        // references in the frame graph, so later pairs must never mutate them.
        ComPtr<ID2D1CommandList> content, provider;
        const auto recordInput = [&](std::uint32_t layerIndex, ComPtr<ID2D1CommandList>& capture) {
            auto native = context->CreateCommandList(&capture);
            if (FAILED(native)) return native;
            context->SetTarget(capture.Get());
            for (const auto& candidate : plan.drawItems)
                if (plan.sourceScene->drawItems[candidate.sourceDrawItemIndex].layerIndex == layerIndex) {
                    if (!issuePrimitive(candidate)) { context->SetTarget(frame.Get()); return static_cast<HRESULT>(result.error.nativeCode); }
                }
            // Close is valid only after this recording is no longer the target.
            context->SetTarget(frame.Get());
            return capture->Close();
        };
        hr = recordInput(contentIndex,content);
        if (FAILED(hr)) return result.error ? result : failure(hr,"unable to record matte content command list");
        hr = recordInput(providerIndex,provider);
        if (FAILED(hr)) return result.error ? result : failure(hr,"unable to record matte provider command list");
        ComPtr<ID2D1Effect> composite;
        hr = context->CreateEffect(CLSID_D2D1Composite,&composite);
        if (FAILED(hr)) return failure(hr,"unable to create alpha Composite effect");
        composite->SetInput(0,content.Get());
        composite->SetInput(1,provider.Get());
        hr = composite->SetValue(D2D1_COMPOSITE_PROP_MODE,D2D1_COMPOSITE_MODE_DESTINATION_IN);
        if (FAILED(hr)) return failure(hr,"unable to configure alpha Composite effect");
        context->SetTransform(toD2D(render::Matrix3x2::identity()));
        context->DrawImage(composite.Get(),nullptr,nullptr,D2D1_INTERPOLATION_MODE_LINEAR,D2D1_COMPOSITE_MODE_SOURCE_OVER);
    }
    context->SetTarget(targetGuard.original.Get());
    hr = frame->Close();
    if (FAILED(hr)) return failure(hr,"unable to close isolated frame command list");
    context->SetTransform(toD2D(render::Matrix3x2::identity()));
    context->DrawImage(frame.Get(),nullptr,nullptr,D2D1_INTERPOLATION_MODE_LINEAR,D2D1_COMPOSITE_MODE_SOURCE_OVER);
    return result;
}

void Backend::invalidateGraphicsDomain(
    std::uint64_t graphicsDomainId,
    std::uint64_t graphicsGeneration) noexcept {
    if (impl_->graphicsDomainId == graphicsDomainId
        && impl_->graphicsGeneration == graphicsGeneration) {
        impl_->resetResources();
    }
}

void Backend::clear() noexcept {
    impl_->resetResources();
    impl_->graphicsDomainId = 0;
    impl_->graphicsGeneration = 0;
}

BackendDiagnostics Backend::diagnostics() const noexcept {
    return impl_->diagnostics;
}

} // namespace avemotion::backends::direct2d
