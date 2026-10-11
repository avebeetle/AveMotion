#pragma once
#include "avemotion/backends/direct2d/Direct2DBackend.hpp"

namespace avemotion::backends::direct2d::detail {
// Fixed private successor; public Backend::draw keeps its skip policy.
class AlphaMatteBackendAccess final {
public:
    [[nodiscard]] static DrawResult draw(Backend& backend,
        const render::MotionRenderPlan& plan, const RenderSession& session);
};
} // namespace avemotion::backends::direct2d::detail
