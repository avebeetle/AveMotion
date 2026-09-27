#include "OwnNativeEllipseModel.hpp"

#include "OwnJsonReader.hpp"

#include "avemotion/core/Hash.hpp"
#include "avemotion/evaluation/PropertyEvaluator.hpp"

#include <array>
#include <cstddef>
#include <optional>
#include <span>
#include <string_view>
#include <utility>

namespace avemotion::runtime::detail {
namespace {

using namespace model;

std::uint64_t rawHash(std::string_view text) {
    const auto bytes = std::span<const std::byte>{
        reinterpret_cast<const std::byte*>(text.data()), text.size()};
    return core::fnv1a64(bytes);
}

std::uint64_t namedHash(const std::string& text) {
    core::Fnv1a64 hash;
    hash.appendString(text);
    return hash.value();
}

std::string effectiveName(const std::optional<std::string>& name, std::string fallback) {
    return name && !name->empty() ? *name : std::move(fallback);
}

MotionValueRef value(PropertyValueType type, std::uint32_t index) {
    return {type, index};
}

std::shared_ptr<const MotionAssetModel> buildModel(
    const NativeEllipseInput& input, const NativeEllipseNumericValues& values,
    std::string_view exactJson) {
    auto model = std::make_shared<MotionAssetModel>();
    model->revision = 1;
    model->sourceAssetHash = rawHash(exactJson);
    model->logicalWidth = input.width;
    model->logicalHeight = input.height;
    model->frameRate = static_cast<double>(values.frameRate);
    model->totalFrames = input.endFrame;
    model->debugName = input.name.value_or("");

    const auto root = makeId<SourceNodeId>(0);
    const auto layer = makeId<SourceNodeId>(1);
    const auto group = makeId<SourceNodeId>(2);
    const auto ellipse = makeId<SourceNodeId>(3);
    const auto fill = makeId<SourceNodeId>(4);
    const auto composition = makeId<CompositionId>(0);
    model->compositions.push_back({true, composition, root, "root", input.width, input.height,
        0.0, static_cast<double>(input.endFrame), static_cast<double>(values.frameRate)});
    model->sourceChildIds = {layer, group, ellipse, fill};
    for (std::size_t index = 0; index < 5; ++index) {
        MotionSourceNodeRecord node;
        node.present = true;
        node.id = makeId<SourceNodeId>(index);
        node.composition = composition;
        node.authoredStatic = !values.animated || index == 4;
        node.dependencyBits = values.animated && index != 4
            ? StaticDependencyTimeline : StaticDependencyNone;
        node.kind = std::array<SourceNodeKind, 5>{SourceNodeKind::Composition,
            SourceNodeKind::Layer, SourceNodeKind::ShapeGroup, SourceNodeKind::Ellipse,
            SourceNodeKind::Fill}[index];
        node.parent = index == 0 ? SourceNodeId{} : makeId<SourceNodeId>(index == 4 ? 2 : index - 1);
        node.children = index == 0 ? IndexRange{0, 1}
            : index == 1 ? IndexRange{1, 1} : index == 2 ? IndexRange{2, 2} : IndexRange{};
        node.properties = index == 0 ? IndexRange{} : IndexRange{
            static_cast<std::uint32_t>((index - 1) * 2), 2};
        if (index == 1) {
            node.layerKind = SourceLayerKind::Shape;
            node.authoredLayerId = input.layerId;
            node.inFrame = static_cast<double>(input.layerInFrame);
            node.outFrame = static_cast<double>(input.layerOutFrame);
        }
        const auto name = index == 0 ? std::string{"root"}
            : index == 1 ? effectiveName(input.layerName, "layer:" + std::to_string(input.layerId))
            : index == 2 ? effectiveName(input.groupName, "group")
            : index == 3 ? effectiveName(input.ellipseName, "source-node")
            : effectiveName(input.fillName, "source-node");
        node.debugName = name;
        node.nameHash = namedHash(name);
        model->sourceNodes.push_back(std::move(node));
    }

    for (std::size_t index = 0; index < 8; ++index) {
        model->sourcePropertyIds.push_back(makeId<PropertyId>(index));
    }
    const std::array<PropertySemantic, 8> semantics{PropertySemantic::TransformMatrix,
        PropertySemantic::TransformOpacity, PropertySemantic::TransformMatrix,
        PropertySemantic::TransformOpacity, PropertySemantic::EllipsePosition,
        PropertySemantic::EllipseSize, PropertySemantic::FillColor,
        PropertySemantic::FillOpacity};
    const std::array<PropertyValueType, 8> types{PropertyValueType::Matrix3x2,
        PropertyValueType::Scalar, PropertyValueType::Matrix3x2, PropertyValueType::Scalar,
        PropertyValueType::Vec2, PropertyValueType::Vec2, PropertyValueType::Color,
        PropertyValueType::Scalar};
    const std::array<SourceNodeId, 8> owners{layer, layer, group, group, ellipse, ellipse, fill, fill};
    for (std::size_t index = 0; index < 8; ++index) {
        MotionPropertyRecord property;
        property.present = true;
        property.id = makeId<PropertyId>(index);
        property.owner = owners[index];
        property.semantic = semantics[index];
        property.valueType = types[index];
        if (index == 4 && values.animated) {
            property.flags = PropertyFlagAnimated;
            property.track = makeId<TrackId>(0);
        } else {
            property.flags = PropertyFlagStatic;
            property.staticValue = index == 0 ? value(PropertyValueType::Matrix3x2, 0)
                : index == 1 ? value(PropertyValueType::Scalar, 0)
                : index == 2 ? value(PropertyValueType::Matrix3x2, 1)
                : index == 3 ? value(PropertyValueType::Scalar, 1)
                : index == 4 ? value(PropertyValueType::Vec2, 0)
                : index == 5 ? value(PropertyValueType::Vec2, values.animated ? 2 : 1)
                : index == 6 ? value(PropertyValueType::Color, 0)
                : value(PropertyValueType::Scalar, 2);
        }
        model->properties.push_back(std::move(property));
    }
    model->scalarValues = {100.0F, 100.0F, 100.0F};
    if (values.animated) {
        model->vec2Values = {values.start, values.end, values.size};
        model->tracks.push_back({true, makeId<TrackId>(0), makeId<PropertyId>(4), {0, 1},
            static_cast<double>(values.firstFrame), static_cast<double>(values.lastFrame)});
        const auto linear = values.outgoing == MotionVec2Value{0, 0}
            && values.incoming == MotionVec2Value{1, 1};
        model->segments.push_back({true, makeId<SegmentId>(0), makeId<TrackId>(0),
            static_cast<double>(values.firstFrame), static_cast<double>(values.lastFrame),
            linear ? SegmentInterpolation::Linear : SegmentInterpolation::CubicBezier,
            SpatialInterpolation::None, value(PropertyValueType::Vec2, 0),
            value(PropertyValueType::Vec2, 1), values.outgoing, values.incoming, {}, {}});
    } else {
        model->vec2Values = {values.start, values.size};
    }
    model->colorValues = {values.color};
    model->matrixValues = {{1, 0, 0, 1, values.translation.x, values.translation.y},
        {1, 0, 0, 1, 0, 0}};
    model->statistics.directParsedModel = true;
    model->statistics.compositionCount = 1;
    model->statistics.sourceNodeCount = 5;
    model->statistics.propertyCount = 8;
    model->statistics.staticPropertyCount = values.animated ? 7 : 8;
    model->statistics.animatedPropertyCount = values.animated ? 1 : 0;
    model->statistics.trackCount = model->tracks.size();
    model->statistics.segmentCount = model->segments.size();
    model->statistics.scalarValueCount = 3;
    model->statistics.vec2ValueCount = model->vec2Values.size();
    model->statistics.colorValueCount = 1;
    model->statistics.matrixValueCount = 2;
    core::Fnv1a64 fingerprint;
    fingerprint.appendString("AveMotion.OwnEllipse.Authored.v1");
    fingerprint.appendString(exactJson);
    model->parsedModelFingerprint = fingerprint.value();
    return std::const_pointer_cast<const MotionAssetModel>(model);
}

} // namespace

OwnNativeEllipseModel::OwnNativeEllipseModel(
    std::string exactJsonValue, std::shared_ptr<const NativeEllipseInput> inputValue,
    std::shared_ptr<const model::MotionAssetModel> modelValue,
    NativeEllipseNumericValues valuesValue, NativeEllipseModelBinding bindingValue)
    : exactJson(std::move(exactJsonValue)), input(std::move(inputValue)), model(std::move(modelValue)),
      values(std::move(valuesValue)), binding(std::move(bindingValue)) {}

OwnNativeEllipseModelResult buildOwnNativeEllipseModel(
    const formats::detail::OwnJsonDocument& document) {
    OwnNativeEllipseModelResult result;
    const auto admitted = decodeOwnNativeEllipseInput(document);
    result.admission = admitted.admission;
    if (!admitted) {
        result.code = OwnNativeEllipseModelCode::AdmissionRejected;
        return result;
    }
    const auto numeric = interpretNativeEllipseInput(*admitted.input);
    if (!numeric) {
        result.code = OwnNativeEllipseModelCode::UnsupportedNumericConversion;
        return result;
    }
    const auto values = *numeric;
    const auto model = buildModel(*admitted.input, values, document.source());
    const auto binding = bindNativeEllipseModel(*admitted.input, *model);
    if (!binding) return result;
    evaluation::PropertyEvaluator evaluator{model};
    if (!evaluator.valid()) return result;
    evaluation::PropertyEvaluationWorkspace workspace;
    evaluator.prepare(workspace);
    result.code = OwnNativeEllipseModelCode::Prepared;
    result.prepared = std::shared_ptr<const OwnNativeEllipseModel>{new OwnNativeEllipseModel{
        std::string{document.source()}, admitted.input, model, values, *binding.binding}};
    return result;
}

} // namespace avemotion::runtime::detail
