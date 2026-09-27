#include "NativeEllipsePathMaterializer.hpp"

#include "avemotion/core/Hash.hpp"

#include <algorithm>
#include <cmath>

namespace avemotion::render::detail {
namespace {

void includePoint(runtime::RectF& bounds, runtime::Vec2 point) noexcept {
    if (!bounds.valid) {
        bounds = {true, point.x, point.y, point.x, point.y};
        return;
    }
    bounds.left = std::min(bounds.left, point.x);
    bounds.top = std::min(bounds.top, point.y);
    bounds.right = std::max(bounds.right, point.x);
    bounds.bottom = std::max(bounds.bottom, point.y);
}

} // namespace

bool materializeNativeEllipsePath(
    const PrimitivePath& primitive,
    const runtime::AffineTransform* transform,
    runtime::EvaluatedPath& path) {
    if (!primitive.valid) return false;
    path.verbs.assign(primitive.verbSpan().begin(), primitive.verbSpan().end());
    path.points.reserve(primitive.pointCount);
    for (const auto source : primitive.pointSpan()) {
        runtime::Vec2 point{source.x, source.y};
        if (transform) {
            point = {source.x * transform->m11 + source.y * transform->m21 + transform->dx,
                     source.x * transform->m12 + source.y * transform->m22 + transform->dy};
        }
        if (!std::isfinite(point.x) || !std::isfinite(point.y)) return false;
        path.points.push_back(point);
        includePoint(path.controlBounds, point);
    }
    if (path.verbs.empty()) return true;
    core::Fnv1a64 hash;
    hash.appendU64(path.verbs.size());
    hash.appendU64(path.points.size());
    for (auto verb : path.verbs) hash.appendU8(static_cast<std::uint8_t>(verb));
    for (auto point : path.points) {
        hash.appendFloat(point.x);
        hash.appendFloat(point.y);
    }
    path.hash = hash.value();
    return true;
}

} // namespace avemotion::render::detail
