#include "SourcePathMaterializer.hpp"
#include "avemotion/core/Hash.hpp"
#include <algorithm>
#include <cmath>
namespace avemotion::render::detail {
[[nodiscard]] bool resolveShape(const model::MotionAssetModel &modelValue,
                                const model::MotionPropertyRecord &property,
                                const evaluation::PropertyEvaluationView &view,
                                ShapeSample &result) noexcept {
    if (property.valueType != model::PropertyValueType::Shape ||
        property.id.index() >= view.properties.size()) {
        return false;
    }
    const auto &evaluated = view.properties[property.id.index()];
    if (evaluated.id != property.id || !evaluated.value.supported())
        return false;

    if (evaluated.value.storage == evaluation::PropertyStorageKind::AssetReference) {
        const auto reference = evaluated.value.assetReference;
        if (reference.type != model::PropertyValueType::Shape ||
            reference.index >= modelValue.shapeValues.size()) {
            return false;
        }
        const auto &shape = modelValue.shapeValues[reference.index];
        if (shape.points.end() > modelValue.shapePoints.size())
            return false;
        result.points = std::span<const model::MotionVec2Value>{
            modelValue.shapePoints.data() + shape.points.first, shape.points.count};
        result.closed = shape.closed;
        result.animated = false;
        return true;
    }

    if (evaluated.value.storage != evaluation::PropertyStorageKind::Materialized ||
        evaluated.value.shapeSlot >= view.shapes.size()) {
        return false;
    }
    const auto &shape = view.shapes[evaluated.value.shapeSlot];
    if (shape.property != property.id ||
        static_cast<std::size_t>(shape.firstPoint) + shape.pointCount > view.shapePoints.size()) {
        return false;
    }
    result.points = view.shapePoints.subspan(shape.firstPoint, shape.pointCount);
    result.closed = shape.closed;
    result.animated = true;
    return true;
}

[[nodiscard]] bool shapePathStream(const ShapeSample &shape, std::vector<runtime::PathVerb> &verbs,
                                   std::vector<model::MotionVec2Value> &points) {
    if (!shape.points.empty() && (shape.points.size() - 1U) % 3U != 0U) {
        return false;
    }
    points.assign(shape.points.begin(), shape.points.end());
    verbs.clear();
    if (shape.points.empty())
        return true;
    const auto segmentCount = (shape.points.size() - 1U) / 3U;
    verbs.reserve(1U + segmentCount + (shape.closed ? 1U : 0U));
    verbs.push_back(runtime::PathVerb::MoveTo);
    for (std::size_t index = 0U; index < segmentCount; ++index) {
        verbs.push_back(runtime::PathVerb::CubicTo);
    }
    if (shape.closed)
        verbs.push_back(runtime::PathVerb::Close);
    return true;
}

bool materializeSourcePath(std::span<const runtime::PathVerb> verbs,
                           std::span<const model::MotionVec2Value> points,
                           const runtime::AffineTransform &matrix, runtime::EvaluatedPath &output) {
    output = {};
    output.verbs.assign(verbs.begin(), verbs.end());
    core::Fnv1a64 hash;
    hash.appendU64(verbs.size());
    hash.appendU64(points.size());
    for (auto verb : verbs)
        hash.appendU8(static_cast<std::uint8_t>(verb));
    for (auto p : points) {
        runtime::Vec2 point{p.x * matrix.m11 + p.y * matrix.m21 + matrix.dx,
                            p.x * matrix.m12 + p.y * matrix.m22 + matrix.dy};
        if (!std::isfinite(point.x) || !std::isfinite(point.y))
            return false;
        output.points.push_back(point);
        auto &b = output.controlBounds;
        if (!b.valid)
            b = {true, point.x, point.y, point.x, point.y};
        else {
            b.left = std::min(b.left, point.x);
            b.right = std::max(b.right, point.x);
            b.top = std::min(b.top, point.y);
            b.bottom = std::max(b.bottom, point.y);
        }
        hash.appendFloat(point.x);
        hash.appendFloat(point.y);
    }
    output.hash = hash.value();
    return true;
}
} // namespace avemotion::render::detail
