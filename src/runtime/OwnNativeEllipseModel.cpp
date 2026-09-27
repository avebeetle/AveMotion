#include "OwnNativeEllipseModel.hpp"
#include "OwnJsonReader.hpp"
#include "avemotion/core/Hash.hpp"
#include "avemotion/evaluation/PropertyEvaluator.hpp"

#include <algorithm>
#include <span>
#include <utility>

namespace avemotion::runtime::detail {
namespace {
using namespace model;

std::string effectiveName(const std::optional<std::string>& name, std::string fallback) {
    return name && !name->empty() ? *name : std::move(fallback);
}
bool animated(const OwnPrimitiveGroupValues& group) {
    return group.position.animated || group.size.animated
        || (group.roundness && group.roundness->animated);
}

std::shared_ptr<const MotionAssetModel> buildModel(
    const OwnPrimitiveInput& input, const OwnPrimitiveNumericValues& values,
    std::string_view exactJson) {
    auto result = std::make_shared<MotionAssetModel>();
    auto& m = *result;
    m.revision = 1;
    m.sourceAssetHash = core::fnv1a64(std::span<const std::byte>{
        reinterpret_cast<const std::byte*>(exactJson.data()), exactJson.size()});
    m.logicalWidth = input.width;
    m.logicalHeight = input.height;
    m.frameRate = static_cast<double>(values.frameRate);
    m.totalFrames = input.endFrame;
    m.debugName = input.name.value_or("");
    const auto root = makeId<SourceNodeId>(0), layer = makeId<SourceNodeId>(1);
    const auto composition = makeId<CompositionId>(0);
    m.compositions.push_back({true, composition, root, "root", input.width, input.height,
        0.0, static_cast<double>(input.endFrame), static_cast<double>(values.frameRate)});
    const auto appendNode = [&](SourceNodeKind kind, SourceNodeId parent, std::string name,
                                bool moving) {
        MotionSourceNodeRecord node;
        node.present = true;
        node.id = makeId<SourceNodeId>(m.sourceNodes.size());
        node.composition = composition;
        node.kind = kind;
        node.parent = parent;
        node.authoredStatic = !moving;
        node.dependencyBits = moving ? StaticDependencyTimeline : StaticDependencyNone;
        node.debugName = std::move(name);
        core::Fnv1a64 hash;
        hash.appendString(node.debugName);
        node.nameHash = hash.value();
        m.sourceNodes.push_back(std::move(node));
    };
    const bool moving = std::any_of(values.groups.begin(), values.groups.end(), animated);
    appendNode(SourceNodeKind::Composition, {}, "root", moving);
    appendNode(SourceNodeKind::Layer, root,
        effectiveName(input.layerName, "layer:" + std::to_string(input.layerId)), moving);
    m.sourceNodes[0].children = {0, 1};
    m.sourceNodes[1].children = {1, static_cast<std::uint32_t>(input.groups.size())};
    m.sourceNodes[1].layerKind = SourceLayerKind::Shape;
    m.sourceNodes[1].authoredLayerId = input.layerId;
    m.sourceNodes[1].inFrame = input.layerInFrame;
    m.sourceNodes[1].outFrame = input.layerOutFrame;
    m.sourceChildIds.push_back(layer);
    for (std::size_t g = 0; g < input.groups.size(); ++g)
        m.sourceChildIds.push_back(makeId<SourceNodeId>(2+3*g));

    const auto scalar = [&](float v) {
        const auto ref = MotionValueRef{PropertyValueType::Scalar, static_cast<std::uint32_t>(m.scalarValues.size())};
        m.scalarValues.push_back(v); return ref;
    };
    const auto vec2 = [&](MotionVec2Value v) {
        const auto ref = MotionValueRef{PropertyValueType::Vec2, static_cast<std::uint32_t>(m.vec2Values.size())};
        m.vec2Values.push_back(v); return ref;
    };
    const auto matrix = [&](MotionVec2Value translation) {
        const auto ref = MotionValueRef{PropertyValueType::Matrix3x2, static_cast<std::uint32_t>(m.matrixValues.size())};
        m.matrixValues.push_back({1,0,0,1,translation.x,translation.y}); return ref;
    };
    const auto color = [&](MotionColorValue v) {
        const auto ref = MotionValueRef{PropertyValueType::Color, static_cast<std::uint32_t>(m.colorValues.size())};
        m.colorValues.push_back(v); return ref;
    };
    const auto property = [&](SourceNodeId owner, PropertySemantic semantic, MotionValueRef start) {
        MotionPropertyRecord p;
        p.present = true; p.id = makeId<PropertyId>(m.properties.size());
        p.owner = owner; p.semantic = semantic; p.valueType = start.type;
        p.flags = PropertyFlagStatic; p.staticValue = start;
        auto& range = m.sourceNodes[owner.index()].properties;
        if (!range.count) range.first = static_cast<std::uint32_t>(m.sourcePropertyIds.size());
        ++range.count;
        m.sourcePropertyIds.push_back(p.id);
        m.properties.push_back(p);
    };
    const auto animatedProperty = [&](SourceNodeId owner, PropertySemantic semantic,
                                      const auto& v, const auto& appendValue) {
        const auto start = appendValue(v.start);
        property(owner, semantic, start);
        if (!v.animated) return;
        const auto end = appendValue(v.end);
        auto& p = m.properties.back();
        p.flags = PropertyFlagAnimated; p.staticValue = {};
        p.track = makeId<TrackId>(m.tracks.size());
        m.tracks.push_back({true, p.track, p.id,
            {static_cast<std::uint32_t>(m.segments.size()),1},
            static_cast<double>(v.firstFrame),static_cast<double>(v.lastFrame)});
        const bool linear = v.outgoing == MotionVec2Value{0,0} && v.incoming == MotionVec2Value{1,1};
        m.segments.push_back({true,makeId<SegmentId>(m.segments.size()),p.track,
            static_cast<double>(v.firstFrame),static_cast<double>(v.lastFrame),
            linear?SegmentInterpolation::Linear:SegmentInterpolation::CubicBezier,
            SpatialInterpolation::None,start,end,v.outgoing,v.incoming,{},{}});
    };
    property(layer, PropertySemantic::TransformMatrix, matrix(values.translation));
    property(layer, PropertySemantic::TransformOpacity, scalar(100));
    for (std::size_t g = 0; g < input.groups.size(); ++g) {
        const auto& in = input.groups[g];
        const auto& v = values.groups[g];
        const auto group = makeId<SourceNodeId>(m.sourceNodes.size());
        const auto primitive = makeId<SourceNodeId>(group.index()+1);
        const auto fill = makeId<SourceNodeId>(group.index()+2);
        const bool rectangle = in.kind == OwnPrimitiveKind::Rectangle;
        appendNode(SourceNodeKind::ShapeGroup,layer,effectiveName(in.groupName,"group"),animated(v));
        appendNode(rectangle?SourceNodeKind::Rectangle:SourceNodeKind::Ellipse,group,
            effectiveName(in.primitiveName,"source-node"),animated(v));
        appendNode(SourceNodeKind::Fill,group,effectiveName(in.fillName,"source-node"),false);
        m.sourceNodes[primitive.index()].pathDirection = in.direction;
        m.sourceNodes[group.index()].children = {static_cast<std::uint32_t>(m.sourceChildIds.size()),2};
        m.sourceChildIds.push_back(primitive); m.sourceChildIds.push_back(fill);
        property(group,PropertySemantic::TransformMatrix,matrix({}));
        property(group,PropertySemantic::TransformOpacity,scalar(100));
        animatedProperty(primitive,rectangle?PropertySemantic::RectanglePosition:PropertySemantic::EllipsePosition,v.position,vec2);
        animatedProperty(primitive,rectangle?PropertySemantic::RectangleSize:PropertySemantic::EllipseSize,v.size,vec2);
        if (v.roundness) animatedProperty(primitive,PropertySemantic::RectangleRoundness,*v.roundness,scalar);
        property(fill,PropertySemantic::FillColor,color(v.color));
        property(fill,PropertySemantic::FillOpacity,scalar(100));
    }
    auto& stats = m.statistics;
    stats.directParsedModel = true;
    stats.compositionCount = 1; stats.sourceNodeCount = m.sourceNodes.size();
    stats.propertyCount = m.properties.size(); stats.animatedPropertyCount = m.tracks.size();
    stats.staticPropertyCount = stats.propertyCount - stats.animatedPropertyCount;
    stats.trackCount = m.tracks.size(); stats.segmentCount = m.segments.size();
    stats.scalarValueCount = m.scalarValues.size(); stats.vec2ValueCount = m.vec2Values.size();
    stats.colorValueCount = m.colorValues.size(); stats.matrixValueCount = m.matrixValues.size();
    core::Fnv1a64 fingerprint;
    fingerprint.appendString("AveMotion.OwnEllipse.Authored.v1");
    fingerprint.appendString(exactJson);
    m.parsedModelFingerprint = fingerprint.value();
    return result;
}
} // namespace

OwnNativeEllipseModel::OwnNativeEllipseModel(
    std::string exactJsonValue, std::shared_ptr<const OwnPrimitiveInput> inputValue,
    std::shared_ptr<const model::MotionAssetModel> modelValue,
    OwnPrimitiveNumericValues valuesValue, OwnPrimitiveBinding bindingValue)
    : exactJson(std::move(exactJsonValue)), input(std::move(inputValue)), model(std::move(modelValue)),
      values(std::move(valuesValue)), binding(std::move(bindingValue)) {}

OwnNativeEllipseModelResult buildOwnNativeEllipseModel(const formats::detail::OwnJsonDocument& document) {
    OwnNativeEllipseModelResult result;
    const auto admitted = decodeOwnPrimitiveInput(document);
    result.admission = admitted.admission;
    if (!admitted) { result.code = OwnNativeEllipseModelCode::AdmissionRejected; return result; }
    const auto numeric = interpretOwnPrimitiveInput(*admitted.input);
    if (!numeric) { result.code = OwnNativeEllipseModelCode::UnsupportedNumericConversion; return result; }
    const auto model = buildModel(*admitted.input,*numeric,document.source());
    const auto binding = bindOwnPrimitiveModel(*admitted.input,*model);
    if (!binding) return result;
    evaluation::PropertyEvaluator evaluator{model};
    if (!evaluator.valid()) return result;
    evaluation::PropertyEvaluationWorkspace workspace;
    evaluator.prepare(workspace);
    result.code = OwnNativeEllipseModelCode::Prepared;
    result.prepared = std::shared_ptr<const OwnNativeEllipseModel>{new OwnNativeEllipseModel{
        std::string{document.source()},admitted.input,model,*numeric,*binding.binding}};
    return result;
}
} // namespace avemotion::runtime::detail
