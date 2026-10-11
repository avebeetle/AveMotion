#pragma once

#include "avemotion/backends/direct2d/Direct2DBackend.hpp"

namespace avemotion::backends::direct2d::detail {
// Fixed build-tree-only profile. Hosts retain their public draw policy.
class LinearGradientBackendAccess final {
public:
    [[nodiscard]] static DrawResult draw(Backend& backend,
        const render::MotionRenderPlan& plan, const RenderSession& session);
};
} // namespace avemotion::backends::direct2d::detail
