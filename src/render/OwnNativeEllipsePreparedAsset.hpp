#pragma once

#include "OwnNativeEllipseModel.hpp"

#include <memory>

namespace avemotion::render::detail {

enum class OwnNativeEllipsePrepareCode {
    Ready,
    InvalidModel,
    ResourceConstructionFailed,
    ModelConstructionFailed,
};

struct OwnNativeEllipsePrepareResult;

class OwnNativeEllipsePreparedAsset final {
public:
    const std::shared_ptr<const runtime::detail::OwnNativeEllipseModel> authored;
    const std::shared_ptr<const model::MotionAssetModel> model;

private:
    friend struct OwnNativeEllipsePrepareResult;
    friend OwnNativeEllipsePrepareResult prepareOwnNativeEllipseAsset(
        std::shared_ptr<const runtime::detail::OwnNativeEllipseModel>);

    OwnNativeEllipsePreparedAsset(
        std::shared_ptr<const runtime::detail::OwnNativeEllipseModel> authoredValue,
        std::shared_ptr<const model::MotionAssetModel> modelValue);
};

struct OwnNativeEllipsePrepareResult final {
    OwnNativeEllipsePrepareCode code = OwnNativeEllipsePrepareCode::InvalidModel;
    std::shared_ptr<const OwnNativeEllipsePreparedAsset> prepared;

    [[nodiscard]] explicit operator bool() const noexcept;
};

[[nodiscard]] OwnNativeEllipsePrepareResult prepareOwnNativeEllipseAsset(
    std::shared_ptr<const runtime::detail::OwnNativeEllipseModel> authored);

} // namespace avemotion::render::detail
