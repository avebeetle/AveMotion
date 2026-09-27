#pragma once

#include "NativeEllipseInput.hpp"

namespace avemotion::formats::detail {
class OwnJsonDocument;
}

namespace avemotion::runtime::detail {

[[nodiscard]] NativeEllipseInputResult decodeOwnNativeEllipseInput(
    const formats::detail::OwnJsonDocument& document);

} // namespace avemotion::runtime::detail
