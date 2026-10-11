#include <avemotion/core/Hash.hpp>
#include <avemotion/formats/Tgs.hpp>
#include <avemotion/model/AssetModel.hpp>
#include <avemotion/player/Player.hpp>
#include <avemotion/evaluation/PropertyEvaluator.hpp>
#include <avemotion/render/HeadlessBackend.hpp>
#include <avemotion/render/RenderPlan.hpp>
#include <avemotion/render/OwnPlayback.hpp>
#include <avemotion/runtime/Handles.hpp>
#include <avemotion/runtime/Playback.hpp>
#include <avemotion/runtime/Runtime.hpp>
#include <avemotion/validation/AssetValidator.hpp>

#include <array>
#include <cstddef>
#include <memory>
#include <span>
#include <string_view>

int main() {
    using avemotion::runtime::AssetHandle;
    using avemotion::runtime::InstanceHandle;

    const AssetHandle asset{3U, 7U};
    const InstanceHandle instance{5U, 11U};
    if (!asset.valid() || !instance.valid()) {
        return 1;
    }
    if (asset.packed() == instance.packed()) {
        return 2;
    }

    avemotion::model::MotionAssetModel model;
    model.layers.resize(1U);
    model.layers[0].present = true;
    model.layers[0].id = avemotion::model::LayerId{0U};
    if (model.layer(avemotion::model::LayerId{0U}) == nullptr) {
        return 3;
    }
    model.compositions.resize(1U);
    model.compositions[0].present = true;
    model.compositions[0].id = avemotion::model::CompositionId{0U};
    model.properties.resize(1U);
    model.properties[0].present = true;
    model.properties[0].id = avemotion::model::PropertyId{0U};
    model.properties[0].owner = avemotion::model::SourceNodeId{0U};
    model.properties[0].semantic = avemotion::model::PropertySemantic::TransformOpacity;
    model.properties[0].valueType = avemotion::model::PropertyValueType::Scalar;
    model.properties[0].flags = avemotion::model::PropertyFlagStatic;
    if (model.property(avemotion::model::PropertyId{0U}) == nullptr) {
        return 4;
    }

    const auto oneSecond = avemotion::runtime::MotionTime::fromSeconds(1.0);
    if (oneSecond.nanoseconds != 1'000'000'000LL) {
        return 5;
    }

    const auto invalidModel = std::make_shared<avemotion::model::MotionAssetModel>();
    avemotion::evaluation::PropertyEvaluator evaluator{invalidModel};
    if (evaluator.valid()) {
        return 6;
    }
    avemotion::evaluation::PropertyEvaluationWorkspace evaluationWorkspace;
    evaluator.prepare(evaluationWorkspace);
    if (evaluationWorkspace.propertyCount() != 0U) {
        return 7;
    }

    avemotion::player::Player player;
    const auto emptyTick = player.tick(
        avemotion::runtime::MotionTime::fromNanoseconds(0));
    if (!emptyTick.frames.empty() || emptyTick.nextDeadline.has_value()) {
        return 8;
    }

    avemotion::validation::AssetValidator validator;
    const auto validationReport = validator.validate(model);
    if (validationReport.structurallyValid) {
        return 9;
    }

    const avemotion::render::MotionRenderPlan plan;
    avemotion::render::HeadlessPlanBackend backend;
    const auto record = backend.record(plan);
    static_cast<void>(record);

    constexpr std::array<std::byte, 9> bytes{
        std::byte{'A'}, std::byte{'v'}, std::byte{'e'}, std::byte{'M'},
        std::byte{'o'}, std::byte{'t'}, std::byte{'i'}, std::byte{'o'},
        std::byte{'n'}};
    constexpr std::array<std::byte, 2> gzipMagic{
        std::byte{0x1F}, std::byte{0x8B}};
    if (avemotion::formats::detectAssetFormat(gzipMagic)
        != avemotion::formats::AssetFormat::TelegramTgs) {
        return 10;
    }
    if (avemotion::core::fnv1a64(std::span<const std::byte>{bytes}) == 0U) {
        return 11;
    }

    avemotion::runtime::Runtime runtime;
    const auto loaded = runtime.loadLottieJson(
        R"({"v":"5.7.4","fr":30,"ip":0,"op":2,"w":16,"h":16,"layers":[]})",
        "offline-consumer");
    if (loaded || loaded.error.code !=
        avemotion::runtime::RuntimeErrorCode::ReferenceUnavailable) {
        return 12;
    }
    constexpr std::string_view ownJson = R"({"fr":60,"ip":0,"op":61,"w":128,"h":128,"layers":[{"ddd":0,"ty":4,"ind":1,"ip":0,"op":61,"ks":{},"shapes":[{"ty":"gr","it":[{"ty":"sh","ks":{"a":0,"k":{"i":[[0,0],[0,0]],"o":[[0,0],[0,0]],"v":[[0,0],[20,0]],"c":false}}},{"ty":"st","c":{"a":0,"k":[1,0,0,1]},"w":{"a":0,"k":2}},{"ty":"tr"}]}]}]})";
    const auto prepared = avemotion::render::prepareOwnAsset(
        std::as_bytes(std::span{ownJson.data(), ownJson.size()}));
    if (!prepared || prepared.asset->metadata().logicalWidth != 128) return 13;
    const auto created = avemotion::render::createOwnPlayback(prepared.asset);
    if (!created) return 14;
    auto own = created.playback;
    own->play(oneSecond);
    const auto ownAdded = avemotion::render::addOwnPlayback(player, own, oneSecond);
    if (!ownAdded || avemotion::render::ownPlayback(player, ownAdded.handle) != own) return 15;
    const auto ownTick = player.tick(oneSecond, avemotion::player::FrameSelection::AllVisible);
    if (ownTick.frames.size() != 1 || ownTick.frames[0].instance != nullptr ||
        ownTick.frames[0].instanceHandle.valid()) return 16;
    const auto ownFrame = own->evaluateAt(oneSecond, 128, 128);
    if (!ownFrame || ownFrame.scene->instanceId == 0) return 17;
    return 0;
}
