#pragma once
#include "NativeEllipseBinding.hpp"
#include "OwnPrimitiveInput.hpp"

namespace avemotion::runtime::detail {
struct OwnPrimitiveGroupBinding final {
    model::SourceNodeId group, primitive, fill;
    model::PropertyId groupTransform, groupOpacity, position, size, color, fillOpacity;
    std::optional<model::PropertyId> roundness;
};
struct OwnPrimitiveBinding final {
    model::SourceNodeId root, layer;
    model::PropertyId layerTransform, layerOpacity;
    std::vector<OwnPrimitiveGroupBinding> groups;
};
struct OwnPrimitiveBindingResult final {
    NativeEllipseBindingCode code = NativeEllipseBindingCode::InvalidModelTable;
    std::optional<OwnPrimitiveBinding> binding;
    [[nodiscard]] explicit operator bool() const noexcept {
        return code == NativeEllipseBindingCode::Bound && binding.has_value();
    }
};
[[nodiscard]] OwnPrimitiveBindingResult bindOwnPrimitiveModel(
    const OwnPrimitiveInput&, const model::MotionAssetModel&);
} // namespace avemotion::runtime::detail
