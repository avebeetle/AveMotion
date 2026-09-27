#pragma once

#include "OwnPrimitiveInput.hpp"

#include <optional>
#include <vector>

namespace avemotion::runtime::detail {

struct OwnPrimitiveVectorValues final {
    model::MotionVec2Value start, end, incoming, outgoing;
    bool animated = false;
    std::uint32_t firstFrame = 0, lastFrame = 0;
};

struct OwnPrimitiveScalarValues final {
    float start = 0, end = 0;
    model::MotionVec2Value incoming, outgoing;
    bool animated = false;
    std::uint32_t firstFrame = 0, lastFrame = 0;
};

struct OwnPrimitiveGroupValues final {
    OwnPrimitiveVectorValues position, size;
    std::optional<OwnPrimitiveScalarValues> roundness;
    model::MotionColorValue color;
};

struct OwnPrimitiveNumericValues final {
    float frameRate = 0;
    model::MotionVec2Value translation;
    std::vector<OwnPrimitiveGroupValues> groups;
};

[[nodiscard]] std::optional<OwnPrimitiveNumericValues> interpretOwnPrimitiveInput(
    const OwnPrimitiveInput& input);

} // namespace avemotion::runtime::detail
