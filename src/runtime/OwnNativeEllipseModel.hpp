#pragma once

#include "OwnPrimitiveBinding.hpp"
#include "OwnPrimitiveNumeric.hpp"
#include "OwnNativeEllipseAdmission.hpp"

#include <memory>
#include <string>

namespace avemotion::formats::detail {
class OwnJsonDocument;
}

namespace avemotion::runtime::detail {

struct OwnNativeEllipseModelResult;

enum class OwnNativeEllipseModelCode {
    Prepared,
    AdmissionRejected,
    UnsupportedNumericConversion,
    ModelConstructionFailed,
};

class OwnNativeEllipseModel final {
public:
    const std::string exactJson;
    const std::shared_ptr<const OwnPrimitiveInput> input;
    const std::shared_ptr<const model::MotionAssetModel> model;
    const OwnPrimitiveNumericValues values;
    const OwnPrimitiveBinding binding;

private:
    friend OwnNativeEllipseModelResult buildOwnNativeEllipseModel(
        const formats::detail::OwnJsonDocument&);

    OwnNativeEllipseModel(std::string exactJsonValue,
                          std::shared_ptr<const OwnPrimitiveInput> inputValue,
                          std::shared_ptr<const model::MotionAssetModel> modelValue,
                          OwnPrimitiveNumericValues valuesValue,
                          OwnPrimitiveBinding bindingValue);
};

struct OwnNativeEllipseModelResult final {
    OwnNativeEllipseModelCode code = OwnNativeEllipseModelCode::ModelConstructionFailed;
    NativeEllipseAdmission admission;
    std::shared_ptr<const OwnNativeEllipseModel> prepared;

    [[nodiscard]] explicit operator bool() const noexcept {
        return code == OwnNativeEllipseModelCode::Prepared && prepared != nullptr;
    }
};

[[nodiscard]] OwnNativeEllipseModelResult buildOwnNativeEllipseModel(
    const formats::detail::OwnJsonDocument&);

} // namespace avemotion::runtime::detail
