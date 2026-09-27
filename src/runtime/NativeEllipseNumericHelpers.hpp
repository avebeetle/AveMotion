#pragma once

#include "NativeEllipseInput.hpp"
#include "avemotion/model/AssetModel.hpp"

#include <algorithm>
#include <charconv>
#include <cmath>
#include <limits>
#include <string>

namespace avemotion::runtime::detail::numeric {

[[nodiscard]] inline bool canonical(const NativeEllipseDecimal& value) {
    const auto digits = [](const std::string& text) {
        return !text.empty() && std::all_of(text.begin(), text.end(), [](char c) {
            return c >= '0' && c <= '9';
        });
    };
    if (!digits(value.digits) || !digits(value.power.magnitude)) return false;
    if (value.power.magnitude.size() > 1 && value.power.magnitude.front() == '0') return false;
    if (value.power.magnitude == "0" && value.power.negative) return false;
    if (value.digits == "0")
        return !value.negative && !value.power.negative && value.power.magnitude == "0";
    return value.digits.front() != '0' && value.digits.back() != '0';
}

[[nodiscard]] inline bool convert(const NativeEllipseDecimal& value, float& output,
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
        || !std::isfinite(parsed) || std::abs(parsed) > std::numeric_limits<float>::max())
        return false;
    output = static_cast<float>(parsed);
    if (!std::isfinite(output) || (value.digits != "0" && output == 0.0F)
        || (positive && output <= 0.0F)) return false;
    if (output == 0.0F) output = 0.0F;
    return true;
}

[[nodiscard]] inline bool convert(const NativeEllipseVec2& value,
                                  model::MotionVec2Value& output, bool positive = false) {
    return convert(value[0], output.x, positive) && convert(value[1], output.y, positive);
}

} // namespace avemotion::runtime::detail::numeric
