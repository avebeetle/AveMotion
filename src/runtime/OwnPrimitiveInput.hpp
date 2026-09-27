#pragma once

#include "NativeEllipseInput.hpp"
#include "avemotion/model/AssetModel.hpp"

#include <array>
#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace avemotion::runtime::detail {

enum class OwnPrimitiveKind { Ellipse, Rectangle };

struct OwnPrimitiveStaticScalar final {
    NativeEllipseDecimal value;
    bool operator==(const OwnPrimitiveStaticScalar&) const = default;
};

struct OwnPrimitiveAnimatedScalar final {
    std::uint32_t firstFrame = 0, lastFrame = 0;
    NativeEllipseDecimal start, end;
    NativeEllipseVec2 incoming, outgoing;
    bool operator==(const OwnPrimitiveAnimatedScalar&) const = default;
};

using OwnPrimitiveScalar = std::variant<OwnPrimitiveStaticScalar, OwnPrimitiveAnimatedScalar>;

struct OwnPrimitiveGroupInput final {
    OwnPrimitiveKind kind = OwnPrimitiveKind::Ellipse;
    model::SourcePathDirection direction = model::SourcePathDirection::Clockwise;
    NativeEllipsePosition position, size;
    std::optional<OwnPrimitiveScalar> roundness;
    std::array<NativeEllipseDecimal, 4> fillColor;
    std::optional<std::string> groupName, primitiveName, fillName, transformName;
    bool operator==(const OwnPrimitiveGroupInput&) const = default;
};

struct OwnPrimitiveInput final {
    std::uint32_t width = 0, height = 0, endFrame = 0;
    NativeEllipseDecimal frameRate;
    std::int32_t layerId = 0;
    std::uint32_t layerInFrame = 0, layerOutFrame = 0;
    NativeEllipseVec2 layerTranslation;
    std::optional<std::string> version, name, layerName;
    std::vector<OwnPrimitiveGroupInput> groups;
    bool operator==(const OwnPrimitiveInput&) const = default;
};

struct OwnPrimitiveInputResult final {
    NativeEllipseAdmission admission;
    std::shared_ptr<const OwnPrimitiveInput> input;
    [[nodiscard]] explicit operator bool() const noexcept {
        return admission.accepted() && input != nullptr;
    }
};

} // namespace avemotion::runtime::detail
