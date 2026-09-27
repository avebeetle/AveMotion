#pragma once
#include "avemotion/model/AssetModel.hpp"
#include "avemotion/core/Hash.hpp"
#include <cmath>
#include <optional>
#include <string>
#include <utility>

namespace avemotion::runtime::detail::binding_helpers {
using namespace model;

[[nodiscard]] inline bool range(IndexRange value, std::size_t size) noexcept {
    const auto first = static_cast<std::size_t>(value.first);
    return first <= size && static_cast<std::size_t>(value.count) <= size - first;
}

[[nodiscard]] inline std::uint64_t nameHash(const std::string& name) noexcept {
    core::Fnv1a64 hash;
    hash.appendString(name);
    return hash.value();
}

[[nodiscard]] inline std::string effective(const std::optional<std::string>& name,
                                    std::string fallback) {
    return name && !name->empty() ? *name : std::move(fallback);
}

[[nodiscard]] inline bool finite(const MotionVec2Value& value) noexcept {
    return std::isfinite(value.x) && std::isfinite(value.y);
}
[[nodiscard]] inline bool finite(const MotionColorValue& value) noexcept {
    return std::isfinite(value.r) && std::isfinite(value.g)
        && std::isfinite(value.b) && std::isfinite(value.a);
}
[[nodiscard]] inline bool finite(const MotionMatrix3x2Value& value) noexcept {
    return std::isfinite(value.m11) && std::isfinite(value.m12)
        && std::isfinite(value.m21) && std::isfinite(value.m22)
        && std::isfinite(value.dx) && std::isfinite(value.dy);
}

[[nodiscard]] inline bool defaultNode(const MotionSourceNodeRecord& node,
                               bool animated, SourcePathDirection direction) noexcept {
    return node.present && !node.hidden && node.enabled && !node.autoOrient
        && !node.transformParent.valid() && !node.referencedComposition.valid()
        && node.authoredStatic == !animated
        && node.dependencyBits == (animated ? StaticDependencyTimeline : StaticDependencyNone)
        && node.authoredParentLayerId == -1
        && node.matteMode == SourceMatteMode::None
        && node.maskMode == SourceMaskMode::None && !node.maskInverted
        && node.blendMode == SourceBlendMode::Normal
        && node.fillRule == SourceFillRule::Winding
        && node.strokeCap == SourceStrokeCap::Flat
        && node.strokeJoin == SourceStrokeJoin::Miter
        && node.gradientType == SourceGradientType::None
        && node.pathDirection == direction
        && node.polystarType == SourcePolystarType::None
        && node.trimMode == SourceTrimMode::None
        && node.miterLimit == 0.0F && node.repeaterMaximumCopies == 0.0F
        && node.gradientColorPointCount == 0 && node.layerWidth == 0
        && node.layerHeight == 0 && finite(node.solidColor)
        && node.solidColor == MotionColorValue{0, 0, 0, 1}
        && node.sourceAssetRefHash == 0
        && std::isfinite(node.inFrame) && std::isfinite(node.outFrame)
        && std::isfinite(node.startFrame) && std::isfinite(node.timeStretch)
        && node.startFrame == 0.0 && node.timeStretch == 1.0F;
}

[[nodiscard]] inline bool valueRef(const MotionAssetModel& model, MotionValueRef ref,
                            PropertyValueType type) noexcept {
    if (!ref.valid() || ref.type != type) return false;
    switch (type) {
    case PropertyValueType::Scalar: return ref.index < model.scalarValues.size();
    case PropertyValueType::Vec2: return ref.index < model.vec2Values.size();
    case PropertyValueType::Color: return ref.index < model.colorValues.size();
    case PropertyValueType::Matrix3x2: return ref.index < model.matrixValues.size();
    default: return false;
    }
}

[[nodiscard]] inline bool staticValue(const MotionAssetModel& model, const MotionPropertyRecord& property,
                               PropertyValueType type) noexcept {
    return property.flags == PropertyFlagStatic && property.valueType == type
        && !property.track.valid() && valueRef(model, property.staticValue, type);
}
[[nodiscard]] inline bool defaultNode(const MotionSourceNodeRecord& node, bool animated) noexcept {
    return defaultNode(node, animated, SourcePathDirection::Clockwise);
}

} // namespace avemotion::runtime::detail::binding_helpers

