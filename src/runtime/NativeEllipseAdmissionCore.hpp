#pragma once

#include "NativeEllipseInput.hpp"
#include "OwnPrimitiveInput.hpp"

#include <cstdint>
#include <memory>
#include <span>
#include <string_view>

namespace avemotion::runtime::detail {

using NativeEllipseValueId = std::uint32_t;
inline constexpr NativeEllipseValueId NativeEllipseNoValue = UINT32_MAX;
enum class NativeEllipseValueKind : std::uint8_t { Null, Boolean, Number, String, Object, Array };

struct NativeEllipseValue final {
    NativeEllipseValueKind kind = NativeEllipseValueKind::Null;
    bool hasKey = false;
    std::string_view key, scalar;
    NativeEllipseValueId firstChild = NativeEllipseNoValue;
    NativeEllipseValueId nextSibling = NativeEllipseNoValue;
    std::uint32_t childCount = 0;
};

[[nodiscard]] NativeEllipseAdmission evaluateNativeEllipseValues(
    std::span<const NativeEllipseValue> values,
    std::shared_ptr<const NativeEllipseInput>* output = nullptr);

[[nodiscard]] NativeEllipseAdmission evaluateOwnPrimitiveValues(
    std::span<const NativeEllipseValue> values,
    std::shared_ptr<const OwnPrimitiveInput>* output = nullptr);

[[nodiscard]] bool exactOwnPrimitiveBound(const NativeEllipseDecimal& value,
                                          std::int64_t low, std::int64_t high,
                                          bool strictLow = false);

// Private bounded import conversion, shared with the vector compiler. Exact
// domain/integrality checks precede the established finite-float conversion.
[[nodiscard]] bool convertOwnNumericToken(std::string_view token,
    std::int64_t low, std::int64_t high, bool integral, bool strictLow, float& output);

// Numerical equality of valid JSON decimal tokens, before float rounding.
[[nodiscard]] bool equalOwnNumericTokens(std::string_view left, std::string_view right);

} // namespace avemotion::runtime::detail
