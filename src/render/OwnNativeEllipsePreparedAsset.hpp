#pragma once

#include "OwnNativeEllipseModel.hpp"
#include "OwnSceneProgram.hpp"

#include <memory>

namespace avemotion::runtime::detail {
class OwnVectorModel;
}

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
    const std::shared_ptr<const runtime::detail::OwnVectorModel> vectorAuthored;
    const std::shared_ptr<const model::MotionAssetModel> model;
    const std::shared_ptr<const OwnSceneProgram> program;

  private:
    friend struct OwnNativeEllipsePrepareResult;
    friend OwnNativeEllipsePrepareResult
        prepareOwnNativeEllipseAsset(std::shared_ptr<const runtime::detail::OwnNativeEllipseModel>);
    friend OwnNativeEllipsePrepareResult
        prepareOwnVectorAsset(std::shared_ptr<const runtime::detail::OwnVectorModel>);

    OwnNativeEllipsePreparedAsset(
        std::shared_ptr<const runtime::detail::OwnNativeEllipseModel> authoredValue,
        std::shared_ptr<const runtime::detail::OwnVectorModel> vectorValue,
        std::shared_ptr<const model::MotionAssetModel> modelValue,
        std::shared_ptr<const OwnSceneProgram> programValue);
};

struct OwnNativeEllipsePrepareResult final {
    OwnNativeEllipsePrepareCode code = OwnNativeEllipsePrepareCode::InvalidModel;
    std::shared_ptr<const OwnNativeEllipsePreparedAsset> prepared;
    std::string path;
    std::string message;

    [[nodiscard]] explicit operator bool() const noexcept;
};

[[nodiscard]] OwnNativeEllipsePrepareResult prepareOwnNativeEllipseAsset(
    std::shared_ptr<const runtime::detail::OwnNativeEllipseModel> authored);

[[nodiscard]] OwnNativeEllipsePrepareResult
prepareOwnVectorAsset(std::shared_ptr<const runtime::detail::OwnVectorModel> authored);
[[nodiscard]] OwnNativeEllipsePrepareResult
prepareOwnMotionAsset(const formats::detail::OwnJsonDocument &document);

} // namespace avemotion::render::detail
