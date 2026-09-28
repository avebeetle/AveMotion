#include "OwnNativeEllipsePreparedAsset.hpp"

#include "AssetModelBuilder.hpp"
#include "NativeEllipsePathMaterializer.hpp"
#include "OwnPrimitiveBinding.hpp"
#include "OwnVectorModel.hpp"
#include "PrimitivePathGenerator.hpp"

#include "avemotion/core/Hash.hpp"

#include <utility>

namespace avemotion::render::detail {
namespace {

std::uint64_t stringHash(const std::string &value) {
    core::Fnv1a64 hash;
    hash.appendString(value);
    return hash.value();
}

runtime::Color8 colorFrom(const model::MotionColorValue &color) {
    return {static_cast<std::uint8_t>(255.0F * color.r),
            static_cast<std::uint8_t>(255.0F * color.g),
            static_cast<std::uint8_t>(255.0F * color.b), 255U};
}

} // namespace

OwnNativeEllipsePreparedAsset::OwnNativeEllipsePreparedAsset(
    std::shared_ptr<const runtime::detail::OwnNativeEllipseModel> authoredValue,
    std::shared_ptr<const runtime::detail::OwnVectorModel> vectorValue,
    std::shared_ptr<const model::MotionAssetModel> modelValue,
    std::shared_ptr<const OwnSceneProgram> programValue)
    : authored(std::move(authoredValue)), vectorAuthored(std::move(vectorValue)),
      model(std::move(modelValue)), program(std::move(programValue)) {}

OwnNativeEllipsePrepareResult::operator bool() const noexcept {
    return code == OwnNativeEllipsePrepareCode::Ready && prepared != nullptr;
}

OwnNativeEllipsePrepareResult prepareOwnNativeEllipseAsset(
    std::shared_ptr<const runtime::detail::OwnNativeEllipseModel> authored) {
    if (!authored || !authored->input || !authored->model) {
        return {OwnNativeEllipsePrepareCode::InvalidModel, nullptr};
    }

    if (!runtime::detail::bindOwnPrimitiveModel(*authored->input, *authored->model)) {
        return {OwnNativeEllipsePrepareCode::ModelConstructionFailed, nullptr};
    }
    auto asset = std::make_shared<model::MotionAssetModel>(*authored->model);
    const auto count = static_cast<std::uint32_t>(authored->binding.groups.size());
    const auto shapeName = asset->sourceNodes.size() > 1U
                               ? asset->sourceNodes[authored->binding.layer.index()].debugName
                               : std::string{};
    if (shapeName.empty())
        return {OwnNativeEllipsePrepareCode::ModelConstructionFailed, nullptr};

    asset->layers.resize(2U);
    asset->layers[0] = {true,
                        model::makeId<model::LayerId>(0),
                        {},
                        {0, 1},
                        {},
                        {},
                        stringHash("__"),
                        "__",
                        model::StaticDependencyNone,
                        runtime::MatteMode::None};
    asset->layers[1] = {true,
                        model::makeId<model::LayerId>(1),
                        model::makeId<model::LayerId>(0),
                        {},
                        {0, count},
                        {},
                        stringHash(shapeName),
                        shapeName,
                        model::StaticDependencyNone,
                        runtime::MatteMode::None};
    asset->childLayerIds = {model::makeId<model::LayerId>(1)};
    asset->clips.resize(1U);
    asset->clips[0] = {true, model::makeId<model::ClipId>(0),         "default",
                       0.0,  static_cast<double>(asset->totalFrames), model::ClipLoopHint::Loop};

    asset->geometries.resize(count);
    asset->paints.resize(count);
    // Lottie content groups paint in reverse authored order. Slot identity is
    // separate from source-role identity, including for equal resources.
    for (std::uint32_t slot = 0; slot < count; ++slot) {
        const auto g = count - 1U - slot;
        const auto &values = authored->values.groups[g];
        const auto &input = authored->input->groups[g];
        const bool animated = values.position.animated || values.size.animated ||
                              (values.roundness && values.roundness->animated);
        asset->layerNodeIds.push_back(model::makeId<model::NodeId>(slot));
        asset->nodes.push_back(
            {true, model::makeId<model::NodeId>(slot), model::makeId<model::DrawItemId>(slot),
             model::makeId<model::LayerId>(1), model::makeId<model::GeometryId>(slot),
             model::makeId<model::PaintId>(slot), slot,
             model::StaticDependencyTransform |
                 (animated ? model::StaticDependencyGeometry : model::StaticDependencyNone)});
        asset->drawOrder.push_back(model::makeId<model::NodeId>(slot));
        auto &geometry = asset->geometries[slot];
        geometry.present = true;
        geometry.id = model::makeId<model::GeometryId>(slot);
        if (animated) {
            geometry.resourceClass = model::ResourceClass::InstanceEvaluated;
            geometry.contentHash = 0U;
            geometry.staticValue.reset();
        } else {
            if (values.size.start.x / 2.0F == 0.0F || values.size.start.y / 2.0F == 0.0F) {
                return {OwnNativeEllipsePrepareCode::ResourceConstructionFailed, nullptr};
            }
            const auto primitive =
                input.kind == runtime::detail::OwnPrimitiveKind::Rectangle
                    ? generateRectanglePath(values.position.start, values.size.start,
                                            values.roundness->start, input.direction)
                    : generateEllipsePath(values.position.start, values.size.start,
                                          input.direction);
            runtime::EvaluatedPath path;
            if (!materializeNativeEllipsePath(primitive, nullptr, path) || path.verbs.empty() ||
                path.points.empty() || !path.controlBounds.valid) {
                return {OwnNativeEllipsePrepareCode::ResourceConstructionFailed, nullptr};
            }
            const auto hash =
                model::detail::hashCanonicalGeometry(runtime::FillRule::Winding, path);
            geometry.resourceClass = model::ResourceClass::AssetStatic;
            geometry.contentHash = hash;
            geometry.staticValue =
                runtime::CanonicalGeometry{static_cast<std::uint64_t>(geometry.id.value) + 1U, hash,
                                           runtime::FillRule::Winding, std::move(path)};
        }

        auto &paint = asset->paints[slot];
        paint.present = true;
        paint.id = model::makeId<model::PaintId>(slot);
        paint.resourceClass = model::ResourceClass::AssetStatic;
        runtime::EvaluatedStroke stroke;
        runtime::EvaluatedPaint evaluatedPaint;
        evaluatedPaint.kind = runtime::PaintKind::Solid;
        evaluatedPaint.solid = colorFrom(values.color);
        const auto paintHash = model::detail::hashCanonicalPaint(stroke, evaluatedPaint);
        paint.contentHash = paintHash;
        paint.staticValue =
            runtime::CanonicalPaint{static_cast<std::uint64_t>(paint.id.value) + 1U, paintHash,
                                    std::move(stroke), std::move(evaluatedPaint)};
    }

    asset->statistics.declaredLayerCount = 2;
    asset->statistics.declaredNodeCount = count;
    asset->statistics.declaredGeometryCount = count;
    asset->statistics.declaredPaintCount = count;
    model::detail::refreshAssetModelDerivedData(*asset);
    if (!runtime::detail::bindOwnPrimitiveModel(*authored->input, *asset)) {
        return {OwnNativeEllipsePrepareCode::ModelConstructionFailed, nullptr};
    }
    auto frozen = std::const_pointer_cast<const model::MotionAssetModel>(asset);
    auto program = lowerOwnPrimitiveProgram(*authored, *asset);
    return {OwnNativeEllipsePrepareCode::Ready,
            std::shared_ptr<const OwnNativeEllipsePreparedAsset>{new OwnNativeEllipsePreparedAsset{
                std::move(authored), nullptr, std::move(frozen), std::move(program)}}};
}

OwnNativeEllipsePrepareResult
prepareOwnVectorAsset(std::shared_ptr<const runtime::detail::OwnVectorModel> authored) {
    if (!authored || !runtime::detail::validateOwnVectorModel(*authored))
        return {OwnNativeEllipsePrepareCode::InvalidModel, nullptr, "/",
                "invalid own vector model"};
    auto asset = std::make_shared<model::MotionAssetModel>(*authored->model);
    auto program = lowerOwnVectorProgram(*authored, *asset);
    evaluation::PropertyEvaluator evaluator(asset, authored->propertyFrameOffsets);
    evaluation::PropertyEvaluationWorkspace workspace;
    evaluator.prepare(workspace);
    const auto view = evaluator.evaluate(0, workspace);
    if (!view)
        return {OwnNativeEllipsePrepareCode::ModelConstructionFailed, nullptr, "/",
                "vector evaluation preparation failed"};
    for (const auto &draw : program->draws) {
        const auto slot = static_cast<std::uint32_t>(asset->geometries.size());
        model::MotionGeometryRecord geometry;
        geometry.present = true;
        geometry.id = model::makeId<model::GeometryId>(slot);
        // The existing stroke route consumes final-space paths and widths.
        // Its render resources depend on the viewport and layer transform even
        // when the program's source-local shape/paint dependencies are static.
        geometry.resourceClass =
            draw.strokeWidth.valid() ? model::ResourceClass::InstanceEvaluated : draw.geometryClass;
        if (geometry.resourceClass == model::ResourceClass::AssetStatic) {
            runtime::EvaluatedPath local, final;
            if (!materializeOwnScenePath(*asset, draw, view, {}, local, final))
                return {OwnNativeEllipsePrepareCode::ResourceConstructionFailed, nullptr, "/",
                        "vector static path unavailable"};
            geometry.contentHash = model::detail::hashCanonicalGeometry(draw.fillRule, local);
            geometry.staticValue =
                runtime::CanonicalGeometry{static_cast<std::uint64_t>(slot) + 1,
                                           geometry.contentHash, draw.fillRule, std::move(local)};
        }
        asset->geometries.push_back(std::move(geometry));
        runtime::EvaluatedStroke stroke;
        runtime::EvaluatedPaint paint;
        if (!sampleOwnScenePaint(*asset, draw, view, stroke, paint))
            return {OwnNativeEllipsePrepareCode::ResourceConstructionFailed, nullptr, "/",
                    "vector paint unavailable"};
        const bool isStroke = stroke.enabled;
        const auto hash = model::detail::hashCanonicalPaint(stroke, paint);
        if (isStroke) {
            asset->paints.push_back({true, model::makeId<model::PaintId>(slot), 0,
                                     model::ResourceClass::InstanceEvaluated, std::nullopt});
        } else {
            asset->paints.push_back(
                {true, model::makeId<model::PaintId>(slot), hash, model::ResourceClass::AssetStatic,
                 runtime::CanonicalPaint{static_cast<std::uint64_t>(slot) + 1, hash,
                                         std::move(stroke), std::move(paint)}});
        }
    }
    asset->clips = {{true, model::makeId<model::ClipId>(0), "default", 0,
                     static_cast<double>(asset->totalFrames), model::ClipLoopHint::Loop}};
    asset->statistics.declaredLayerCount = asset->layers.size();
    asset->statistics.declaredNodeCount = asset->nodes.size();
    asset->statistics.declaredGeometryCount = asset->geometries.size();
    asset->statistics.declaredPaintCount = asset->paints.size();
    model::detail::refreshAssetModelDerivedData(*asset);
    return {OwnNativeEllipsePrepareCode::Ready,
            std::shared_ptr<const OwnNativeEllipsePreparedAsset>{new OwnNativeEllipsePreparedAsset{
                nullptr, std::move(authored), std::move(asset), std::move(program)}},
            {},
            {}};
}

OwnNativeEllipsePrepareResult
prepareOwnMotionAsset(const formats::detail::OwnJsonDocument &document) {
    // Select exactly once by the authored content vocabulary. A vector input
    // failing admission must never retry the primitive compiler or a renderer.
    bool primitive = false;
    for (std::uint32_t i = 0; i < document.nodes().size(); ++i) {
        if (document.memberName(i) == "ty") {
            const auto value = document.valueBytes(i);
            if (value == "el" || value == "rc")
                primitive = true;
        }
    }
    if (primitive) {
        auto compiled = runtime::detail::buildOwnNativeEllipseModel(document);
        if (!compiled)
            return {OwnNativeEllipsePrepareCode::InvalidModel, nullptr, "/",
                    "own primitive admission rejected"};
        return prepareOwnNativeEllipseAsset(std::move(compiled.prepared));
    }
    auto compiled = runtime::detail::buildOwnVectorModel(document);
    if (!compiled)
        return {OwnNativeEllipsePrepareCode::InvalidModel, nullptr, compiled.path,
                compiled.message};
    return prepareOwnVectorAsset(std::move(compiled.prepared));
}

} // namespace avemotion::render::detail
