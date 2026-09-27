#pragma once

#include "PrimitivePathGenerator.hpp"

namespace avemotion::render::detail {

[[nodiscard]] bool materializeNativeEllipsePath(
    const PrimitivePath& primitive,
    const runtime::AffineTransform* transform,
    runtime::EvaluatedPath& path);

} // namespace avemotion::render::detail
