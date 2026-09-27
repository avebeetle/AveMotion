#pragma once

#include "NativeEllipseInput.hpp"
#include "avemotion/model/AssetModel.hpp"

#include <cstdint>
#include <optional>

namespace avemotion::runtime::detail {

struct NativeEllipseNumericValues final {
    float frameRate = 0;
    model::MotionVec2Value translation, size, start, end, outgoing, incoming;
    model::MotionColorValue color;
    bool animated = false;
    std::uint32_t firstFrame = 0, lastFrame = 0;
};

[[nodiscard]] std::optional<NativeEllipseNumericValues> interpretNativeEllipseInput(
    const NativeEllipseInput&);

} // namespace avemotion::runtime::detail
