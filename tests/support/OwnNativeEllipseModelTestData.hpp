#pragma once

#include "OwnNativeEllipseModel.hpp"
#include "NativeEllipseNumeric.hpp"

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

inline runtime::detail::NativeEllipseInput ownLegacyInput(const runtime::detail::OwnPrimitiveInput& in) {
    using namespace runtime::detail;
    requireOwn(in.groups.size()==1, "checked legacy descriptor has one group");
    const auto& g=in.groups.front();
    requireOwn(g.kind==OwnPrimitiveKind::Ellipse && g.direction==model::SourcePathDirection::Clockwise
        && !g.roundness && std::holds_alternative<NativeEllipseStaticPosition>(g.size),
        "checked legacy descriptor is a clockwise ellipse with static size");
    return {in.width,in.height,in.endFrame,in.frameRate,in.layerId,in.layerInFrame,in.layerOutFrame,
        in.layerTranslation,std::get<NativeEllipseStaticPosition>(g.size).value,g.position,g.fillColor,
        in.version,in.name,in.layerName,g.groupName,g.primitiveName,g.fillName,g.transformName};
}

inline runtime::detail::NativeEllipseNumericValues ownLegacyValues(
    const runtime::detail::OwnPrimitiveNumericValues& in) {
    requireOwn(in.groups.size()==1, "checked legacy numeric view has one group");
    const auto& g=in.groups.front();
    const auto staticInvariant = [](const auto& v) {
        return !v.animated && v.start==v.end && v.firstFrame==0 && v.lastFrame==0
            && v.incoming==model::MotionVec2Value{} && v.outgoing==model::MotionVec2Value{};
    };
    requireOwn(!g.roundness && staticInvariant(g.size), "new static size invariant before projection");
    if (!g.position.animated) requireOwn(staticInvariant(g.position), "new static position invariant before projection");
    runtime::detail::NativeEllipseNumericValues result;
    result.frameRate=in.frameRate; result.translation=in.translation; result.size=g.size.start;
    result.start=g.position.start; result.color=g.color; result.animated=g.position.animated;
    // Legacy static endpoints/controls were unused and default-initialized.
    if (g.position.animated) {
        result.end=g.position.end; result.outgoing=g.position.outgoing; result.incoming=g.position.incoming;
        result.firstFrame=g.position.firstFrame; result.lastFrame=g.position.lastFrame;
    }
    return result;
}

inline runtime::detail::NativeEllipseModelBinding ownLegacyBinding(
    const runtime::detail::OwnPrimitiveBinding& b) {
    requireOwn(b.groups.size()==1 && !b.groups.front().roundness, "checked one-ellipse binding");
    const auto& g=b.groups.front();
    return {b.root,b.layer,g.group,g.primitive,g.fill,b.layerTransform,b.layerOpacity,
        g.groupTransform,g.groupOpacity,g.position,g.size,g.color,g.fillOpacity};
}

inline void ownAppendByte(std::uint64_t& hash, std::uint8_t byte) {
    hash ^= byte;
    hash *= 1099511628211ULL;
}

inline std::uint64_t ownRawHash(std::string_view text) {
    std::uint64_t hash = 14695981039346656037ULL;
    for (const auto byte : text) ownAppendByte(hash, static_cast<std::uint8_t>(byte));
    return hash;
}

inline void ownAppendString(std::uint64_t& hash, std::string_view text) {
    for (std::size_t index = 0; index < sizeof(std::uint64_t); ++index)
        ownAppendByte(hash, static_cast<std::uint8_t>((text.size() >> (index * 8U)) & 0xFFU));
    for (const auto byte : text) ownAppendByte(hash, static_cast<std::uint8_t>(byte));
}

inline std::uint64_t ownStringHash(std::string_view text) {
    std::uint64_t hash = 14695981039346656037ULL;
    ownAppendString(hash, text);
    return hash;
}

inline std::uint64_t ownParsedFingerprint(std::string_view text) {
    std::uint64_t hash = 14695981039346656037ULL;
    ownAppendString(hash, "AveMotion.OwnEllipse.Authored.v1");
    ownAppendString(hash, text);
    return hash;
}

inline void assertOwnAuthoredModel(
    const runtime::detail::OwnNativeEllipseModel& prepared,
    const runtime::detail::NativeEllipseInput& input,
    const runtime::detail::NativeEllipseNumericValues& values,
    std::string_view exactSource) {
    using namespace model;
    const auto& asset = *prepared.model;
    const auto oldInput=ownLegacyInput(*prepared.input);
    const auto oldValues=ownLegacyValues(prepared.values);
    const auto oldBinding=ownLegacyBinding(prepared.binding);
    requireOwn(prepared.input && oldInput == input, "retained admitted input");
    requireOwn(oldInput.version == input.version && oldInput.name == input.name
        && oldInput.layerName == input.layerName && oldInput.groupName == input.groupName
        && oldInput.ellipseName == input.ellipseName && oldInput.fillName == input.fillName
        && oldInput.transformName == input.transformName, "all optional fields retain absent versus empty state");
    requireOwn(oldValues.frameRate == values.frameRate
        && oldValues.translation == values.translation && oldValues.size == values.size
        && oldValues.start == values.start && oldValues.end == values.end
        && oldValues.outgoing == values.outgoing && oldValues.incoming == values.incoming
        && oldValues.color == values.color && oldValues.animated == values.animated
        && oldValues.firstFrame == values.firstFrame && oldValues.lastFrame == values.lastFrame,
        "retained numeric values");
    requireOwn(asset.schemaVersion == 2 && asset.revision == 1 && !asset.assetHandle.valid(),
        "authored metadata identity");
    requireOwn(prepared.exactJson == exactSource, "exact source bytes retained");
    requireOwn(asset.sourceAssetHash == ownRawHash(exactSource), "raw source hash");
    requireOwn(asset.parsedModelFingerprint == ownParsedFingerprint(exactSource),
        "versioned parsed fingerprint");
    requireOwn(ownRawHash(exactSource) != ownStringHash(exactSource),
        "raw source hash differs from length-prefixed string hash");
    requireOwn(asset.topologyFingerprint == 0 && asset.resourceFingerprint == 0 && asset.fingerprint == 0,
        "no render-derived fingerprints");
    requireOwn(asset.logicalWidth == input.width && asset.logicalHeight == input.height
        && asset.totalFrames == input.endFrame && asset.frameRate == static_cast<double>(values.frameRate)
        && asset.debugName == input.name.value_or(""), "authored root metadata");
    requireOwn(asset.layers.empty() && asset.nodes.empty() && asset.geometries.empty()
        && asset.paints.empty() && asset.clips.empty() && asset.drawOrder.empty()
        && asset.childLayerIds.empty() && asset.layerNodeIds.empty(), "no render rows");
    requireOwn(asset.shapeValues.empty() && asset.shapePoints.empty() && asset.gradientValues.empty()
        && asset.gradientFloats.empty(), "unused typed tables empty");
    requireOwn(asset.compositions.size() == 1 && asset.sourceNodes.size() == 5
        && asset.sourceChildIds.size() == 4 && asset.sourcePropertyIds.size() == 8
        && asset.properties.size() == 8 && asset.tracks.size() == (values.animated ? 1U : 0U)
        && asset.segments.size() == (values.animated ? 1U : 0U), "authored table sizes");
    const auto& composition = asset.compositions[0];
    requireOwn(composition.present && composition.id == makeId<CompositionId>(0)
        && composition.rootNode == makeId<SourceNodeId>(0) && composition.debugName == "root"
        && composition.logicalWidth == input.width && composition.logicalHeight == input.height
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
            && !node.maskInverted && node.blendMode == SourceBlendMode::Normal
            && node.fillRule == SourceFillRule::Winding && node.strokeCap == SourceStrokeCap::Flat
            && node.strokeJoin == SourceStrokeJoin::Miter && node.gradientType == SourceGradientType::None
            && node.pathDirection == SourcePathDirection::Clockwise
            && node.polystarType == SourcePolystarType::None && node.trimMode == SourceTrimMode::None
            && node.miterLimit == 0.0F && node.repeaterMaximumCopies == 0.0F
            && node.gradientColorPointCount == 0 && node.layerWidth == 0 && node.layerHeight == 0
            && node.solidColor == MotionColorValue{0, 0, 0, 1} && node.sourceAssetRefHash == 0
            && node.inFrame == (index == 1 ? static_cast<double>(input.layerInFrame) : 0.0)
            && node.outFrame == (index == 1 ? static_cast<double>(input.layerOutFrame) : 0.0)
            && node.startFrame == 0.0 && node.timeStretch == 1.0F
            && node.layerKind == (index == 1 ? SourceLayerKind::Shape : SourceLayerKind::None)
            && node.authoredLayerId == (index == 1 ? input.layerId : -1),
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
        const auto expectedStatic = !(values.animated && index == 4);
        requireOwn(property.flags == (expectedStatic ? PropertyFlagStatic : PropertyFlagAnimated)
            && (expectedStatic ? !property.track.valid() : property.track == makeId<TrackId>(0)),
            "property storage mode");
    }
    const std::array<MotionValueRef, 8> staticRefs{
        MotionValueRef{PropertyValueType::Matrix3x2, 0}, MotionValueRef{PropertyValueType::Scalar, 0},
        MotionValueRef{PropertyValueType::Matrix3x2, 1}, MotionValueRef{PropertyValueType::Scalar, 1},
        MotionValueRef{PropertyValueType::Vec2, 0}, MotionValueRef{PropertyValueType::Vec2, values.animated ? 2U : 1U},
        MotionValueRef{PropertyValueType::Color, 0}, MotionValueRef{PropertyValueType::Scalar, 2}};
    for (std::size_t index = 0; index < staticRefs.size(); ++index) {
        requireOwn(values.animated && index == 4 ? !asset.properties[index].staticValue.valid()
            : asset.properties[index].staticValue == staticRefs[index], "property value reference");
    }
    requireOwn(asset.scalarValues == std::vector<float>{100, 100, 100}
        && asset.colorValues == std::vector<MotionColorValue>{values.color}
        && asset.matrixValues == std::vector<MotionMatrix3x2Value>{{1, 0, 0, 1, values.translation.x, values.translation.y}, {1, 0, 0, 1, 0, 0}},
        "static typed values");
    const auto expectedVec2 = values.animated
        ? std::vector<MotionVec2Value>{values.start, values.end, values.size}
        : std::vector<MotionVec2Value>{values.start, values.size};
    requireOwn(asset.vec2Values == expectedVec2, "vec2 typed values");
    if (values.animated) {
        const auto& track = asset.tracks[0];
        const auto& segment = asset.segments[0];
        const auto linear = values.outgoing == MotionVec2Value{0, 0}
            && values.incoming == MotionVec2Value{1, 1};
        requireOwn(track.present && track.id == makeId<TrackId>(0) && track.property == makeId<PropertyId>(4)
            && track.segments.first == 0 && track.segments.count == 1
            && track.firstFrame == static_cast<double>(values.firstFrame)
            && track.endFrame == static_cast<double>(values.lastFrame), "track row");
        requireOwn(segment.present && segment.id == makeId<SegmentId>(0) && segment.track == track.id
            && segment.firstFrame == static_cast<double>(values.firstFrame)
            && segment.endFrame == static_cast<double>(values.lastFrame)
            && segment.interpolation == (linear ? SegmentInterpolation::Linear : SegmentInterpolation::CubicBezier)
            && segment.spatialInterpolation == SpatialInterpolation::None
            && segment.startValue == MotionValueRef{PropertyValueType::Vec2, 0}
            && segment.endValue == MotionValueRef{PropertyValueType::Vec2, 1}
            && segment.temporalControl1 == values.outgoing && segment.temporalControl2 == values.incoming
            && segment.spatialInTangent == MotionVec2Value{} && segment.spatialOutTangent == MotionVec2Value{},
            "segment row");
    }
    requireOwn(asset.statistics.directParsedModel && asset.statistics.compositionCount == 1
        && asset.statistics.sourceNodeCount == 5 && asset.statistics.propertyCount == 8
        && asset.statistics.staticPropertyCount == (values.animated ? 7U : 8U)
        && asset.statistics.animatedPropertyCount == (values.animated ? 1U : 0U)
        && asset.statistics.trackCount == asset.tracks.size() && asset.statistics.segmentCount == asset.segments.size(),
        "authored statistics");
    requireOwn(asset.statistics.declaredLayerCount == 0 && asset.statistics.declaredNodeCount == 0
        && asset.statistics.declaredGeometryCount == 0 && asset.statistics.declaredPaintCount == 0
        && asset.statistics.observedLayerCount == 0 && asset.statistics.observedNodeCount == 0
        && asset.statistics.observedGeometryCount == 0 && asset.statistics.observedPaintCount == 0
        && asset.statistics.assetStaticGeometryCount == 0 && asset.statistics.assetStaticPaintCount == 0
        && asset.statistics.maskCount == 0 && asset.statistics.clipCount == 0
        && asset.statistics.scalarValueCount == 3 && asset.statistics.vec2ValueCount == expectedVec2.size()
        && asset.statistics.colorValueCount == 1 && asset.statistics.matrixValueCount == 2
        && asset.statistics.shapeValueCount == 0 && asset.statistics.gradientValueCount == 0,
        "complete authored statistics");
    requireOwn(oldBinding.root == makeId<SourceNodeId>(0)
        && oldBinding.layer == makeId<SourceNodeId>(1) && oldBinding.group == makeId<SourceNodeId>(2)
        && oldBinding.ellipse == makeId<SourceNodeId>(3) && oldBinding.fill == makeId<SourceNodeId>(4)
        && oldBinding.layerTransform == makeId<PropertyId>(0)
        && oldBinding.layerOpacity == makeId<PropertyId>(1)
        && oldBinding.groupTransform == makeId<PropertyId>(2)
        && oldBinding.groupOpacity == makeId<PropertyId>(3)
        && oldBinding.position == makeId<PropertyId>(4) && oldBinding.size == makeId<PropertyId>(5)
        && oldBinding.color == makeId<PropertyId>(6) && oldBinding.fillOpacity == makeId<PropertyId>(7),
        "bound semantic identifiers");
}

} // namespace avemotion::test
