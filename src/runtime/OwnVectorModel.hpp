#pragma once
#include "OwnJsonReader.hpp"
#include "avemotion/model/AssetModel.hpp"

namespace avemotion::runtime::detail {
struct OwnVectorLayerBinding final {
    model::SourceNodeId layer, structuralParent;
    double inFrame = 0, outFrame = 0;
};
struct OwnVectorDrawBinding final {
    model::SourceNodeId layer, group, path, paint;
    std::optional<model::SourceNodeId> trim;
};
struct OwnVectorSourceBinding final {
    model::SourceNodeId source, containingInstance;
    std::string jsonPointer;
    std::int32_t frameOffset = 0;
};
struct OwnVectorModelResult;
class OwnVectorModel final {
public:
    const std::string exactJson;
    const std::shared_ptr<const model::MotionAssetModel> model;
    const std::vector<OwnVectorLayerBinding> layers;
    const std::vector<OwnVectorDrawBinding> draws;
    const std::vector<OwnVectorSourceBinding> sources;
    const std::vector<std::int32_t> propertyFrameOffsets;

private:
    OwnVectorModel(std::string, std::shared_ptr<const model::MotionAssetModel>,
                   std::vector<OwnVectorLayerBinding>, std::vector<OwnVectorDrawBinding>,
                   std::vector<OwnVectorSourceBinding>, std::vector<std::int32_t>);
    friend OwnVectorModelResult buildOwnVectorModel(const formats::detail::OwnJsonDocument&);
};
struct OwnVectorModelResult final {
    std::shared_ptr<const OwnVectorModel> prepared;
    std::string path, message;
    explicit operator bool() const noexcept {
        return prepared != nullptr && message.empty();
    }
};
OwnVectorModelResult buildOwnVectorModel(const formats::detail::OwnJsonDocument&);
bool validateOwnVectorModel(const OwnVectorModel&);
} // namespace avemotion::runtime::detail
