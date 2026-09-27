#include "NativeEllipseNumeric.hpp"

#include <algorithm>
#include <charconv>
#include <cmath>
#include <limits>
#include <string>

namespace avemotion::runtime::detail {
namespace {

[[nodiscard]] bool canonical(const NativeEllipseDecimal& value) {
    const auto digits = [](const std::string& text) {
        return !text.empty() && std::all_of(text.begin(), text.end(), [](char c) {
            return c >= '0' && c <= '9';
        });
    };
    if (!digits(value.digits) || !digits(value.power.magnitude)) return false;
    if (value.power.magnitude.size() > 1 && value.power.magnitude.front() == '0') return false;
    if (value.power.magnitude == "0" && value.power.negative) return false;
    if (value.digits == "0") {
        return !value.negative && !value.power.negative && value.power.magnitude == "0";
    }
    return value.digits.front() != '0' && value.digits.back() != '0';
}

[[nodiscard]] bool convert(const NativeEllipseDecimal& value, float& output,
                           bool positive = false) {
    if (!canonical(value)) return false;
    std::string token;
    token.reserve(value.digits.size() + value.power.magnitude.size() + 3);
    if (value.negative) token += '-';
    token += value.digits;
    token += 'e';
    token += value.power.negative ? '-' : '+';
    token += value.power.magnitude;
    double parsed = 0.0;
    const auto converted = std::from_chars(token.data(), token.data() + token.size(), parsed);
    if (converted.ec != std::errc{} || converted.ptr != token.data() + token.size()
        || !std::isfinite(parsed) || std::abs(parsed) > std::numeric_limits<float>::max()) {
        return false;
    }
    output = static_cast<float>(parsed);
    if (!std::isfinite(output) || (value.digits != "0" && output == 0.0F)
        || (positive && output <= 0.0F)) return false;
    if (output == 0.0F) output = 0.0F;
    return true;
}

[[nodiscard]] bool convert(const NativeEllipseVec2& value, model::MotionVec2Value& output,
                           bool positive = false) {
    return convert(value[0], output.x, positive) && convert(value[1], output.y, positive);
}

} // namespace

std::optional<NativeEllipseNumericValues> interpretNativeEllipseInput(
    const NativeEllipseInput& input) {
    NativeEllipseNumericValues values;
    if (input.width == 0 || input.width > 8192 || input.height == 0 || input.height > 8192
        || input.endFrame < 2 || input.endFrame > 10000 || input.layerId < 1
        || input.layerInFrame >= input.layerOutFrame || input.layerOutFrame > input.endFrame
        || !convert(input.frameRate, values.frameRate, true)
        || !convert(input.layerTranslation, values.translation)
        || !convert(input.size, values.size, true)
        || !convert(input.fillColor[0], values.color.r)
        || !convert(input.fillColor[1], values.color.g)
        || !convert(input.fillColor[2], values.color.b)
        || !convert(input.fillColor[3], values.color.a)
        || values.frameRate > 240.0F
        || std::abs(values.translation.x) > 32768.0F
        || std::abs(values.translation.y) > 32768.0F
        || values.size.x > 16384.0F || values.size.y > 16384.0F
        || values.color.r < 0.0F || values.color.r > 1.0F
        || values.color.g < 0.0F || values.color.g > 1.0F
        || values.color.b < 0.0F || values.color.b > 1.0F
        || values.color.a != 1.0F) return std::nullopt;
    values.animated = std::holds_alternative<NativeEllipseAnimatedPosition>(input.position);
    if (values.animated) {
        const auto& motion = std::get<NativeEllipseAnimatedPosition>(input.position);
        values.firstFrame = motion.firstFrame;
        values.lastFrame = motion.lastFrame;
        if (motion.firstFrame != 0 || motion.lastFrame != input.endFrame - 1
            || !convert(motion.start, values.start) || !convert(motion.end, values.end)
            || !convert(motion.outgoing, values.outgoing) || !convert(motion.incoming, values.incoming)
            || std::abs(values.start.x) > 32768.0F || std::abs(values.start.y) > 32768.0F
            || std::abs(values.end.x) > 32768.0F || std::abs(values.end.y) > 32768.0F
            || values.outgoing.x < 0.0F || values.outgoing.x > 1.0F
            || values.outgoing.y < 0.0F || values.outgoing.y > 1.0F
            || values.incoming.x < 0.0F || values.incoming.x > 1.0F
            || values.incoming.y < 0.0F || values.incoming.y > 1.0F) return std::nullopt;
        return values;
    }
    if (!convert(std::get<NativeEllipseStaticPosition>(input.position).value, values.start)
        || std::abs(values.start.x) > 32768.0F || std::abs(values.start.y) > 32768.0F) {
        return std::nullopt;
    }
    return values;
}

} // namespace avemotion::runtime::detail
