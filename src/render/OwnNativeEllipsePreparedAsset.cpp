#include "OwnNativeEllipsePreparedAsset.hpp"

#include "AssetModelBuilder.hpp"
#include "NativeEllipseBinding.hpp"
#include "NativeEllipsePathMaterializer.hpp"
#include "PrimitivePathGenerator.hpp"

#include "avemotion/core/Hash.hpp"

#include <utility>

namespace avemotion::render::detail {
namespace {

std::uint64_t stringHash(const std::string& value) {
    core::Fnv1a64 hash;
    hash.appendString(value);
    return hash.value();
}

runtime::Color8 colorFrom(const model::MotionColorValue& color) {
    return {static_cast<std::uint8_t>(255.0F * color.r),
            static_cast<std::uint8_t>(255.0F * color.g),
            static_cast<std::uint8_t>(255.0F * color.b), 255U};
}

} // namespace

OwnNativeEllipsePreparedAsset::OwnNativeEllipsePreparedAsset(
    std::shared_ptr<const runtime::detail::OwnNativeEllipseModel> authoredValue,
    std::shared_ptr<const model::MotionAssetModel> modelValue)
    : authored(std::move(authoredValue)), model(std::move(modelValue)) {}

OwnNativeEllipsePrepareResult::operator bool() const noexcept {
    return code == OwnNativeEllipsePrepareCode::Ready && prepared != nullptr;
}

OwnNativeEllipsePrepareResult prepareOwnNativeEllipseAsset(
    std::shared_ptr<const runtime::detail::OwnNativeEllipseModel> authored) {
    if (!authored || !authored->input || !authored->model) {
        return {OwnNativeEllipsePrepareCode::InvalidModel, nullptr};
    }

    auto asset = std::make_shared<model::MotionAssetModel>(*authored->model);
    const bool animated = authored->values.animated;
    const auto shapeName = asset->sourceNodes.size() > 1U
        ? asset->sourceNodes[1].debugName : std::string{};
    if (shapeName.empty()) return {OwnNativeEllipsePrepareCode::ModelConstructionFailed, nullptr};

    asset->layers.resize(2U);
    asset->layers[0] = {true, model::makeId<model::LayerId>(0), {}, {0, 1}, {}, {},
        stringHash("__"), "__", model::StaticDependencyNone, runtime::MatteMode::None};
    asset->layers[1] = {true, model::makeId<model::LayerId>(1), model::makeId<model::LayerId>(0),
        {}, {0, 1}, {}, stringHash(shapeName), shapeName, model::StaticDependencyNone,
        runtime::MatteMode::None};
    asset->childLayerIds = {model::makeId<model::LayerId>(1)};
    asset->layerNodeIds = {model::makeId<model::NodeId>(0)};
    asset->nodes.resize(1U);
    asset->nodes[0] = {true, model::makeId<model::NodeId>(0), model::makeId<model::DrawItemId>(0),
        model::makeId<model::LayerId>(1), model::makeId<model::GeometryId>(0),
        model::makeId<model::PaintId>(0), 0U, model::StaticDependencyTransform
            | (animated ? model::StaticDependencyGeometry : model::StaticDependencyNone)};
    asset->drawOrder = {model::makeId<model::NodeId>(0)};
    asset->clips.resize(1U);
    asset->clips[0] = {true, model::makeId<model::ClipId>(0), "default", 0.0,
        static_cast<double>(asset->totalFrames), model::ClipLoopHint::Loop};

    asset->geometries.resize(1U);
    auto& geometry = asset->geometries[0];
    geometry.present = true;
    geometry.id = model::makeId<model::GeometryId>(0);
    if (animated) {
        geometry.resourceClass = model::ResourceClass::InstanceEvaluated;
        geometry.contentHash = 0U;
        geometry.staticValue.reset();
    } else {
        if (authored->values.size.x / 2.0F == 0.0F
            || authored->values.size.y / 2.0F == 0.0F) {
            return {OwnNativeEllipsePrepareCode::ResourceConstructionFailed, nullptr};
        }
        const auto primitive = generateEllipsePath(authored->values.start, authored->values.size,
            model::SourcePathDirection::Clockwise);
        runtime::EvaluatedPath path;
        if (!materializeNativeEllipsePath(primitive, nullptr, path)
            || path.verbs.empty() || path.points.empty() || !path.controlBounds.valid) {
            return {OwnNativeEllipsePrepareCode::ResourceConstructionFailed, nullptr};
        }
        const auto hash = model::detail::hashCanonicalGeometry(runtime::FillRule::Winding, path);
        geometry.resourceClass = model::ResourceClass::AssetStatic;
        geometry.contentHash = hash;
        geometry.staticValue = runtime::CanonicalGeometry{1U, hash, runtime::FillRule::Winding,
            std::move(path)};
    }

    asset->paints.resize(1U);
    auto& paint = asset->paints[0];
    paint.present = true;
    paint.id = model::makeId<model::PaintId>(0);
    paint.resourceClass = model::ResourceClass::AssetStatic;
    runtime::EvaluatedStroke stroke;
    runtime::EvaluatedPaint evaluatedPaint;
    evaluatedPaint.kind = runtime::PaintKind::Solid;
    evaluatedPaint.solid = colorFrom(authored->values.color);
    const auto paintHash = model::detail::hashCanonicalPaint(stroke, evaluatedPaint);
    paint.contentHash = paintHash;
    paint.staticValue = runtime::CanonicalPaint{1U, paintHash, std::move(stroke),
        std::move(evaluatedPaint)};

    asset->statistics.declaredLayerCount = 2;
    asset->statistics.declaredNodeCount = 1;
    asset->statistics.declaredGeometryCount = 1;
    asset->statistics.declaredPaintCount = 1;
    model::detail::refreshAssetModelDerivedData(*asset);
    if (!runtime::detail::bindNativeEllipseModel(*authored->input, *asset)) {
        return {OwnNativeEllipsePrepareCode::ModelConstructionFailed, nullptr};
    }
    auto frozen = std::const_pointer_cast<const model::MotionAssetModel>(asset);
    return {OwnNativeEllipsePrepareCode::Ready,
        std::shared_ptr<const OwnNativeEllipsePreparedAsset>{new OwnNativeEllipsePreparedAsset{
            std::move(authored), std::move(frozen)}}};
}

} // namespace avemotion::render::detail
