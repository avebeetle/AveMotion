#pragma once

#include "NativeEllipseBinding.hpp"
#include "NativeEllipseNumeric.hpp"
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
    const std::shared_ptr<const NativeEllipseInput> input;
    const std::shared_ptr<const model::MotionAssetModel> model;
    const NativeEllipseNumericValues values;
    const NativeEllipseModelBinding binding;

private:
    friend OwnNativeEllipseModelResult buildOwnNativeEllipseModel(
        const formats::detail::OwnJsonDocument&);

    OwnNativeEllipseModel(std::string exactJsonValue,
                          std::shared_ptr<const NativeEllipseInput> inputValue,
                          std::shared_ptr<const model::MotionAssetModel> modelValue,
                          NativeEllipseNumericValues valuesValue,
                          NativeEllipseModelBinding bindingValue);
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
