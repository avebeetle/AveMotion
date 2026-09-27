#include "OwnPrimitiveNumeric.hpp"
#include "NativeEllipseAdmissionCore.hpp"
#include "NativeEllipseNumericHelpers.hpp"

#include <cmath>

namespace avemotion::runtime::detail {

namespace {
using numeric::convert;

bool within(model::MotionVec2Value value, float lower, float upper, bool strictLower = false) {
    return (strictLower ? value.x > lower && value.y > lower
                        : value.x >= lower && value.y >= lower)
        && value.x <= upper && value.y <= upper;
}

bool exactVectorBound(const NativeEllipseVec2& value, std::int64_t lower,
                      std::int64_t upper, bool strictLower = false) {
    return exactOwnPrimitiveBound(value[0], lower, upper, strictLower)
        && exactOwnPrimitiveBound(value[1], lower, upper, strictLower);
}

bool vectorValues(const NativeEllipsePosition& input, std::uint32_t last,
                  std::int64_t lower, std::int64_t upper, bool strictLower,
                  OwnPrimitiveVectorValues& output) {
    output.animated = std::holds_alternative<NativeEllipseAnimatedPosition>(input);
    if (!output.animated) {
        const auto& source = std::get<NativeEllipseStaticPosition>(input).value;
        if (!exactVectorBound(source, lower, upper, strictLower)
            || !convert(source, output.start)) return false;
        output.end = output.start;
        return true;
    }
    const auto& motion = std::get<NativeEllipseAnimatedPosition>(input);
    output.firstFrame = motion.firstFrame;
    output.lastFrame = motion.lastFrame;
    return motion.firstFrame == 0 && motion.lastFrame == last
        && exactVectorBound(motion.start, lower, upper, strictLower)
        && exactVectorBound(motion.end, lower, upper, strictLower)
        && exactVectorBound(motion.incoming, 0, 1)
        && exactVectorBound(motion.outgoing, 0, 1)
        && convert(motion.start, output.start)
        && convert(motion.end, output.end)
        && convert(motion.incoming, output.incoming)
        && convert(motion.outgoing, output.outgoing)
        && within(output.start, static_cast<float>(lower), static_cast<float>(upper), strictLower)
        && within(output.end, static_cast<float>(lower), static_cast<float>(upper), strictLower);
}

bool scalarValues(const OwnPrimitiveScalar& input, std::uint32_t last,
                  OwnPrimitiveScalarValues& output) {
    output.animated = std::holds_alternative<OwnPrimitiveAnimatedScalar>(input);
    if (!output.animated) {
        const auto& source = std::get<OwnPrimitiveStaticScalar>(input).value;
        if (!exactOwnPrimitiveBound(source, 0, 16384)
            || !convert(source, output.start)
            || output.start < 0.0F || output.start > 16384.0F) return false;
        output.end = output.start;
        return true;
    }
    const auto& motion = std::get<OwnPrimitiveAnimatedScalar>(input);
    output.firstFrame = motion.firstFrame;
    output.lastFrame = motion.lastFrame;
    return motion.firstFrame == 0 && motion.lastFrame == last
        && exactOwnPrimitiveBound(motion.start, 0, 16384)
        && exactOwnPrimitiveBound(motion.end, 0, 16384)
        && exactVectorBound(motion.incoming, 0, 1)
        && exactVectorBound(motion.outgoing, 0, 1)
        && convert(motion.start, output.start)
        && convert(motion.end, output.end)
        && convert(motion.incoming, output.incoming)
        && convert(motion.outgoing, output.outgoing)
        && output.start >= 0.0F && output.start <= 16384.0F
        && output.end >= 0.0F && output.end <= 16384.0F;
}
} // namespace

std::optional<OwnPrimitiveNumericValues> interpretOwnPrimitiveInput(const OwnPrimitiveInput& input) {
    OwnPrimitiveNumericValues values;
    if (input.width < 1 || input.width > 8192 || input.height < 1 || input.height > 8192
        || input.endFrame < 2 || input.endFrame > 10000 || input.layerId < 1
        || input.layerInFrame >= input.layerOutFrame || input.layerOutFrame > input.endFrame
        || input.groups.empty() || input.groups.size() > 16
        || !exactOwnPrimitiveBound(input.frameRate, 0, 240, true)
        || !convert(input.frameRate, values.frameRate, true)
        || values.frameRate > 240.0F
        || !exactVectorBound(input.layerTranslation, -32768, 32768)
        || !convert(input.layerTranslation, values.translation)
        || !within(values.translation, -32768.0F, 32768.0F)) return std::nullopt;
    values.groups.reserve(input.groups.size());
    for (const auto& group : input.groups) {
        OwnPrimitiveGroupValues item;
        if ((group.direction != model::SourcePathDirection::Clockwise
             && group.direction != model::SourcePathDirection::CounterClockwise)
            || !vectorValues(group.position, input.endFrame - 1,
                             -32768, 32768, false, item.position)
            || !vectorValues(group.size, input.endFrame - 1,
                             0, 16384, true, item.size)
            || !exactOwnPrimitiveBound(group.fillColor[0], 0, 1)
            || !exactOwnPrimitiveBound(group.fillColor[1], 0, 1)
            || !exactOwnPrimitiveBound(group.fillColor[2], 0, 1)
            || !exactOwnPrimitiveBound(group.fillColor[3], 1, 1)
            || !convert(group.fillColor[0], item.color.r)
            || !convert(group.fillColor[1], item.color.g)
            || !convert(group.fillColor[2], item.color.b)
            || !convert(group.fillColor[3], item.color.a)
            || item.color.r < 0.0F || item.color.r > 1.0F
            || item.color.g < 0.0F || item.color.g > 1.0F
            || item.color.b < 0.0F || item.color.b > 1.0F
            || item.color.a != 1.0F) return std::nullopt;
        if (group.kind == OwnPrimitiveKind::Rectangle) {
            if (!group.roundness) return std::nullopt;
            item.roundness.emplace();
            if (!scalarValues(*group.roundness, input.endFrame - 1, *item.roundness))
                return std::nullopt;
        } else if (group.kind != OwnPrimitiveKind::Ellipse || group.roundness) {
            return std::nullopt;
        }
        values.groups.push_back(std::move(item));
    }
    return values;
}

} // namespace avemotion::runtime::detail
