#pragma once

#include "NativeEllipseInput.hpp"
#include "OwnPrimitiveInput.hpp"

namespace avemotion::formats::detail {
class OwnJsonDocument;
}

namespace avemotion::runtime::detail {

[[nodiscard]] NativeEllipseInputResult decodeOwnNativeEllipseInput(
    const formats::detail::OwnJsonDocument& document);

[[nodiscard]] OwnPrimitiveInputResult decodeOwnPrimitiveInput(
    const formats::detail::OwnJsonDocument& document);

} // namespace avemotion::runtime::detail
