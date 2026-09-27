#pragma once

#include "OwnNativeEllipseModel.hpp"

#include "avemotion/core/Hash.hpp"

#include <cstddef>
#include <array>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace avemotion::test {

inline void requireOwn(bool value, std::string_view message) {
    if (!value) throw std::runtime_error(std::string{message});
}

inline std::uint64_t ownRawHash(std::string_view text) {
    return core::fnv1a64(std::span<const std::byte>{
        reinterpret_cast<const std::byte*>(text.data()), text.size()});
}

inline std::uint64_t ownStringHash(std::string_view text) {
    core::Fnv1a64 hash;
    hash.appendString(text);
    return hash.value();
}

inline std::uint64_t ownParsedFingerprint(std::string_view text) {
    core::Fnv1a64 hash;
    hash.appendString("AveMotion.OwnEllipse.Authored.v1");
    hash.appendString(text);
    return hash.value();
}

inline void assertOwnAuthoredModel(
    const runtime::detail::OwnNativeEllipseModel& prepared,
    const runtime::detail::NativeEllipseInput& input,
    const runtime::detail::NativeEllipseNumericValues& values) {
    using namespace model;
    const auto& asset = *prepared.model;
    requireOwn(prepared.input && *prepared.input == input, "retained admitted input");
    requireOwn(asset.schemaVersion == 2 && asset.revision == 1 && !asset.assetHandle.valid(),
        "authored metadata identity");
    requireOwn(asset.sourceAssetHash == ownRawHash(prepared.exactJson), "raw source hash");
    requireOwn(asset.parsedModelFingerprint == ownParsedFingerprint(prepared.exactJson),
        "versioned parsed fingerprint");
    requireOwn(asset.topologyFingerprint == 0 && asset.resourceFingerprint == 0 && asset.fingerprint == 0,
        "no render-derived fingerprints");
    requireOwn(asset.logicalWidth == input.width && asset.logicalHeight == input.height
        && asset.totalFrames == input.endFrame && asset.frameRate == static_cast<double>(values.frameRate)
        && asset.debugName == input.name.value_or(""), "authored root metadata");
    requireOwn(asset.layers.empty() && asset.nodes.empty() && asset.geometries.empty()
        && asset.paints.empty() && asset.clips.empty() && asset.drawOrder.empty()
        && asset.childLayerIds.empty() && asset.layerNodeIds.empty(), "no render rows");
    requireOwn(asset.compositions.size() == 1 && asset.sourceNodes.size() == 5
        && asset.sourceChildIds.size() == 4 && asset.sourcePropertyIds.size() == 8
        && asset.properties.size() == 8 && asset.tracks.size() == (values.animated ? 1U : 0U)
        && asset.segments.size() == (values.animated ? 1U : 0U), "authored table sizes");
    const auto& composition = asset.compositions[0];
    requireOwn(composition.present && composition.id == makeId<CompositionId>(0)
        && composition.rootNode == makeId<SourceNodeId>(0) && composition.debugName == "root"
        && composition.firstFrame == 0.0 && composition.endFrame == static_cast<double>(input.endFrame)
        && composition.frameRate == static_cast<double>(values.frameRate), "composition row");
    const std::array<SourceNodeKind, 5> kinds{SourceNodeKind::Composition, SourceNodeKind::Layer,
        SourceNodeKind::ShapeGroup, SourceNodeKind::Ellipse, SourceNodeKind::Fill};
    const std::array<std::string, 5> names{"root",
        input.layerName && !input.layerName->empty() ? *input.layerName : "layer:" + std::to_string(input.layerId),
        input.groupName && !input.groupName->empty() ? *input.groupName : "group",
        input.ellipseName && !input.ellipseName->empty() ? *input.ellipseName : "source-node",
        input.fillName && !input.fillName->empty() ? *input.fillName : "source-node"};
    const std::array<IndexRange, 5> children{IndexRange{0, 1}, IndexRange{1, 1},
        IndexRange{2, 2}, IndexRange{}, IndexRange{}};
    const std::array<IndexRange, 5> properties{IndexRange{}, IndexRange{0, 2},
        IndexRange{2, 2}, IndexRange{4, 2}, IndexRange{6, 2}};
    for (std::size_t index = 0; index < 5; ++index) {
        const auto& node = asset.sourceNodes[index];
        requireOwn(node.present && node.id == makeId<SourceNodeId>(index) && node.kind == kinds[index]
            && node.composition == composition.id && node.children.first == children[index].first
            && node.children.count == children[index].count && node.properties.first == properties[index].first
            && node.properties.count == properties[index].count && node.debugName == names[index]
            && node.nameHash == ownStringHash(names[index]), "source node row");
        requireOwn(node.parent == (index == 0 ? SourceNodeId{} : makeId<SourceNodeId>(index == 4 ? 2 : index - 1))
            && node.authoredStatic == (!values.animated || index == 4)
            && node.dependencyBits == (values.animated && index != 4 ? StaticDependencyTimeline : StaticDependencyNone)
            && !node.hidden && node.enabled && !node.autoOrient && !node.transformParent.valid()
            && !node.referencedComposition.valid() && node.authoredParentLayerId == -1
            && node.matteMode == SourceMatteMode::None && node.maskMode == SourceMaskMode::None
            && node.blendMode == SourceBlendMode::Normal && node.solidColor == MotionColorValue{0, 0, 0, 1},
            "source node defaults");
    }
    const auto& layer = asset.sourceNodes[1];
    requireOwn(layer.layerKind == SourceLayerKind::Shape && layer.authoredLayerId == input.layerId
        && layer.inFrame == static_cast<double>(input.layerInFrame)
        && layer.outFrame == static_cast<double>(input.layerOutFrame), "authored layer identity");
    for (std::size_t index = 0; index < 4; ++index)
        requireOwn(asset.sourceChildIds[index] == makeId<SourceNodeId>(index + 1), "child edge");
    const std::array<PropertySemantic, 8> semantics{PropertySemantic::TransformMatrix,
        PropertySemantic::TransformOpacity, PropertySemantic::TransformMatrix,
        PropertySemantic::TransformOpacity, PropertySemantic::EllipsePosition,
        PropertySemantic::EllipseSize, PropertySemantic::FillColor, PropertySemantic::FillOpacity};
    const std::array<PropertyValueType, 8> types{PropertyValueType::Matrix3x2,
        PropertyValueType::Scalar, PropertyValueType::Matrix3x2, PropertyValueType::Scalar,
        PropertyValueType::Vec2, PropertyValueType::Vec2, PropertyValueType::Color,
        PropertyValueType::Scalar};
    for (std::size_t index = 0; index < 8; ++index) {
        const auto& property = asset.properties[index];
        requireOwn(asset.sourcePropertyIds[index] == makeId<PropertyId>(index)
            && property.present && property.id == makeId<PropertyId>(index)
            && property.owner == makeId<SourceNodeId>(1 + index / 2) && property.semantic == semantics[index]
            && property.semanticIndex == 0 && property.valueType == types[index], "property row");
    }
    requireOwn(asset.scalarValues == std::vector<float>{100, 100, 100}
        && asset.colorValues == std::vector<MotionColorValue>{values.color}
        && asset.matrixValues == std::vector<MotionMatrix3x2Value>{{1, 0, 0, 1, values.translation.x, values.translation.y}, {1, 0, 0, 1, 0, 0}},
        "static typed values");
    requireOwn(asset.statistics.directParsedModel && asset.statistics.compositionCount == 1
        && asset.statistics.sourceNodeCount == 5 && asset.statistics.propertyCount == 8
        && asset.statistics.staticPropertyCount == (values.animated ? 7U : 8U)
        && asset.statistics.animatedPropertyCount == (values.animated ? 1U : 0U)
        && asset.statistics.trackCount == asset.tracks.size() && asset.statistics.segmentCount == asset.segments.size(),
        "authored statistics");
}

} // namespace avemotion::test
