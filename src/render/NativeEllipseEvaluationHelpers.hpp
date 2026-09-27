#pragma once

#include "avemotion/evaluation/PropertyEvaluator.hpp"
#include "avemotion/runtime/EvaluatedScene.hpp"

#include <algorithm>
#include <cstddef>
#include <cmath>
#include <optional>

namespace avemotion::render::detail {

[[nodiscard]] inline bool resolveNativeEllipseVec2(
    const model::MotionAssetModel& model, const evaluation::PropertyEvaluationView& view,
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

[[nodiscard]] inline std::optional<runtime::AffineTransform> nativeEllipseViewportTransform(
    const model::MotionMatrix3x2Value& world, std::size_t logicalWidth,
    std::size_t logicalHeight, std::size_t width, std::size_t height) noexcept {
    const float scale = std::min(float(width) / float(logicalWidth),
                                 float(height) / float(logicalHeight));
    const float tx = (float(width) - float(logicalWidth) * scale) / 2.0F;
    const float ty = (float(height) - float(logicalHeight) * scale) / 2.0F;
    const runtime::AffineTransform transform{world.m11 * scale, world.m12 * scale,
        world.m21 * scale, world.m22 * scale, world.dx * scale + tx,
        world.dy * scale + ty};
    if (!std::isfinite(transform.m11) || !std::isfinite(transform.m12)
        || !std::isfinite(transform.m21) || !std::isfinite(transform.m22)
        || !std::isfinite(transform.dx) || !std::isfinite(transform.dy)) return std::nullopt;
    return transform;
}

} // namespace avemotion::render::detail
