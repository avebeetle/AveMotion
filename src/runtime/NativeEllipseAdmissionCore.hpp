#pragma once

#include "NativeEllipseInput.hpp"

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

} // namespace avemotion::runtime::detail
