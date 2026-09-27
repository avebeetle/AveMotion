#include "AssetModelBuilder.hpp"
#include "NativeEllipseAdmissionTestData.hpp"
#include "NativeEllipseCertificate.hpp"
#include "OwnJsonReader.hpp"
#include "OwnNativeEllipseModel.hpp"
#include "OwnNativeEllipseModelTestData.hpp"
#include "OwnNativeEllipsePreparedAsset.hpp"

#include "avemotion/evaluation/PropertyEvaluator.hpp"

#include <algorithm>
#include <array>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

using namespace avemotion;
using namespace avemotion::model;

struct Counters final {
    std::size_t assertions = 0;
    std::size_t commonCases = 0;
    std::size_t evaluatorSamples = 0;
    std::size_t policyRows = 0;
    std::size_t comparatorWitnesses = 0;
} counters;

void require(bool condition, std::string_view message) {
    ++counters.assertions;
    if (!condition) throw std::runtime_error(std::string{message});
}

std::string replace(std::string source, std::string_view from, std::string_view to) {
    try {
        return test::replaceEllipseOnce(std::move(source), from, to);
    } catch (const std::logic_error&) {
        throw std::runtime_error("replacement not unique: " + std::string{from});
    }
}

std::string staticOriginUnitSource() {
    auto source = test::staticEllipseFixture(test::ellipseFixture());
    source = replace(std::move(source), "[-32768,32768]", "[0,0]");
    source = replace(std::move(source), "[120, 120]", "[2, 2]");
    return replace(std::move(source), "[0.08, 0.72, 0.95, 1]", "[0.5, 0.1, 0.999, 1]");
}

std::string linearSource() {
    auto source = test::ellipseFixture();
    source = replace(std::move(source), "\"x\": 0.667", "\"x\": 1");
    return replace(std::move(source), "\"x\": 0.333", "\"x\": 0");
}

std::string activeSource() {
    auto source = test::ellipseFixture();
    source = replace(std::move(source), "      \"ip\": 0,", "      \"ip\": 10,");
    return replace(std::move(source), "      \"op\": 61,", "      \"op\": 20,");
}

std::string absentNamesSource() {
    auto source = test::ellipseFixture();
    source = replace(std::move(source), ",\n  \"nm\": \"AveMotion Telegram sticker profile fixture\"", "");
    source = replace(std::move(source), ",\n      \"nm\": \"Moving Circle\"", "");
    source = replace(std::move(source), ",\n          \"nm\": \"Circle Group\"", "");
    source = replace(std::move(source), ",\n              \"nm\": \"Animated Ellipse\"", "");
    source = replace(std::move(source), ",\n              \"nm\": \"Fill\"", "");
    return replace(std::move(source), ",\n              \"nm\": \"Transform\"", "");
}

std::string emptyNamesSource() {
    std::string source = test::ellipseFixture();
    for (const auto& name : {std::string{"AveMotion Telegram sticker profile fixture"},
             std::string{"Moving Circle"}, std::string{"Circle Group"},
             std::string{"Animated Ellipse"}, std::string{"Fill"}, std::string{"Transform"}}) {
        source = replace(std::move(source), "\"" + name + "\"", "\"\"");
    }
    return source;
}

struct ValueSnapshot final {
    PropertyValueType type = PropertyValueType::None;
    float scalar = 0.0F;
    MotionVec2Value vec2;
    MotionColorValue color;
    MotionMatrix3x2Value matrix;

    [[nodiscard]] friend bool operator==(const ValueSnapshot&, const ValueSnapshot&) noexcept = default;
};

struct SegmentSnapshot final {
    bool present = false;
    double firstFrame = 0.0;
    double endFrame = 0.0;
    SegmentInterpolation interpolation = SegmentInterpolation::Hold;
    SpatialInterpolation spatialInterpolation = SpatialInterpolation::None;
    ValueSnapshot start;
    ValueSnapshot end;
    MotionVec2Value temporalControl1;
    MotionVec2Value temporalControl2;

    [[nodiscard]] friend bool operator==(const SegmentSnapshot&, const SegmentSnapshot&) noexcept = default;
};

struct SourceRoleSnapshot final {
    std::string role;
    SourceNodeKind kind = SourceNodeKind::Unknown;
    SourceLayerKind layerKind = SourceLayerKind::None;
    std::string parentRole;
    std::string transformParentRole;
    bool referencesComposition = false;
    std::vector<std::string> childRoles;
    std::string debugName;
    bool hidden = false;
    bool enabled = false;
    bool authoredStatic = false;
    bool autoOrient = false;
    std::int32_t authoredLayerId = -1;
    std::int32_t authoredParentLayerId = -1;
    double inFrame = 0.0;
    double outFrame = 0.0;
    double startFrame = 0.0;
    float timeStretch = 1.0F;
    std::uint32_t dependencyBits = StaticDependencyNone;
    SourceFillRule fillRule = SourceFillRule::Winding;
    SourceStrokeCap strokeCap = SourceStrokeCap::Flat;
    SourceStrokeJoin strokeJoin = SourceStrokeJoin::Miter;
    SourceGradientType gradientType = SourceGradientType::None;
    SourceMaskMode maskMode = SourceMaskMode::None;
    SourceMatteMode matteMode = SourceMatteMode::None;
    SourceBlendMode blendMode = SourceBlendMode::Normal;
    SourcePathDirection pathDirection = SourcePathDirection::Clockwise;
    SourcePolystarType polystarType = SourcePolystarType::None;
    SourceTrimMode trimMode = SourceTrimMode::None;
    float miterLimit = 0.0F;
    float repeaterMaximumCopies = 0.0F;
    std::int32_t gradientColorPointCount = 0;
    std::int32_t layerWidth = 0;
    std::int32_t layerHeight = 0;
    MotionColorValue solidColor;
    bool hasSourceAssetReference = false;
    bool maskInverted = false;

    [[nodiscard]] friend bool operator==(const SourceRoleSnapshot&, const SourceRoleSnapshot&) noexcept = default;
};

struct PropertyRoleSnapshot final {
    std::string role;
    std::string ownerRole;
    PropertySemantic semantic = PropertySemantic::Unknown;
    std::uint16_t semanticIndex = 0;
    PropertyValueType valueType = PropertyValueType::None;
    std::uint32_t flags = PropertyFlagNone;
    bool hasStaticValue = false;
    ValueSnapshot staticValue;
    SegmentSnapshot segment;

    [[nodiscard]] friend bool operator==(const PropertyRoleSnapshot&, const PropertyRoleSnapshot&) noexcept = default;
};

struct PathSnapshot final {
    runtime::FillRule fillRule = runtime::FillRule::Winding;
    std::vector<runtime::PathVerb> verbs;
    std::vector<runtime::Vec2> points;
    runtime::RectF bounds;

    [[nodiscard]] friend bool operator==(const PathSnapshot& left, const PathSnapshot& right) noexcept {
        if (left.fillRule != right.fillRule || left.verbs != right.verbs
            || left.points.size() != right.points.size()
            || left.bounds.valid != right.bounds.valid || left.bounds.left != right.bounds.left
            || left.bounds.top != right.bounds.top || left.bounds.right != right.bounds.right
            || left.bounds.bottom != right.bounds.bottom) return false;
        for (std::size_t index = 0; index < left.points.size(); ++index) {
            if (left.points[index].x != right.points[index].x || left.points[index].y != right.points[index].y)
                return false;
        }
        return true;
    }
};

struct PaintSnapshot final {
    bool strokeEnabled = false;
    float strokeWidth = 0.0F;
    runtime::LineCap strokeCap = runtime::LineCap::Flat;
    runtime::LineJoin strokeJoin = runtime::LineJoin::Miter;
    runtime::PaintKind kind = runtime::PaintKind::None;
    runtime::Color8 solid;

    [[nodiscard]] friend bool operator==(const PaintSnapshot& left, const PaintSnapshot& right) noexcept {
        return left.strokeEnabled == right.strokeEnabled && left.strokeWidth == right.strokeWidth
            && left.strokeCap == right.strokeCap && left.strokeJoin == right.strokeJoin
            && left.kind == right.kind && left.solid.r == right.solid.r
            && left.solid.g == right.solid.g && left.solid.b == right.solid.b
            && left.solid.a == right.solid.a;
    }
};

struct ResourceSnapshot final {
    ResourceClass geometryClass = ResourceClass::Unknown;
    bool hasStaticGeometry = false;
    PathSnapshot geometry;
    ResourceClass paintClass = ResourceClass::Unknown;
    bool hasStaticPaint = false;
    PaintSnapshot paint;
    std::uint32_t renderDependencyBits = StaticDependencyNone;

    [[nodiscard]] friend bool operator==(const ResourceSnapshot& left, const ResourceSnapshot& right) noexcept {
        return left.geometryClass == right.geometryClass
            && left.hasStaticGeometry == right.hasStaticGeometry
            && (!left.hasStaticGeometry || left.geometry == right.geometry)
            && left.paintClass == right.paintClass
            && left.hasStaticPaint == right.hasStaticPaint
            && (!left.hasStaticPaint || left.paint == right.paint)
            && left.renderDependencyBits == right.renderDependencyBits;
    }
};

struct EvaluationSample final {
    double frame = 0.0;
    ValueSnapshot position;
    MotionMatrix3x2Value layerLocal;
    MotionMatrix3x2Value layerWorld;
    MotionMatrix3x2Value groupLocal;
    MotionMatrix3x2Value groupWorld;
    float layerLocalOpacity = 1.0F;
    float layerWorldOpacity = 1.0F;
    float groupLocalOpacity = 1.0F;
    float groupWorldOpacity = 1.0F;

    [[nodiscard]] friend bool operator==(const EvaluationSample&, const EvaluationSample&) noexcept = default;
};

struct SemanticSnapshot final {
    std::size_t width = 0;
    std::size_t height = 0;
    std::size_t totalFrames = 0;
    double frameRate = 0.0;
    std::vector<SourceRoleSnapshot> sources;
    std::vector<PropertyRoleSnapshot> properties;
    ResourceSnapshot resources;
    std::vector<EvaluationSample> samples;
};

struct BoundIds final {
    SourceNodeId root, layer, group, ellipse, fill;
    PropertyId layerTransform, layerOpacity, groupTransform, groupOpacity;
    PropertyId position, size, color, fillOpacity;
    LayerId rootLayer, shapeLayer;
    NodeId renderNode;
    GeometryId geometry;
    PaintId paint;
};

ValueSnapshot valueFromRef(const MotionAssetModel& model, MotionValueRef ref) {
    require(ref.valid(), "value ref valid");
    ValueSnapshot value;
    value.type = ref.type;
    switch (ref.type) {
    case PropertyValueType::Scalar:
        require(ref.index < model.scalarValues.size(), "scalar value ref in range");
        value.scalar = model.scalarValues[ref.index];
        break;
    case PropertyValueType::Vec2:
        require(ref.index < model.vec2Values.size(), "vec2 value ref in range");
        value.vec2 = model.vec2Values[ref.index];
        break;
    case PropertyValueType::Color:
        require(ref.index < model.colorValues.size(), "color value ref in range");
        value.color = model.colorValues[ref.index];
        break;
    case PropertyValueType::Matrix3x2:
        require(ref.index < model.matrixValues.size(), "matrix value ref in range");
        value.matrix = model.matrixValues[ref.index];
        break;
    default:
        throw std::runtime_error("unsupported semantic test value type");
    }
    return value;
}

ValueSnapshot valueFromEvaluated(const evaluation::MotionPropertyValue& value) {
    ValueSnapshot result;
    result.type = value.type;
    result.scalar = value.scalar;
    result.vec2 = value.vec2;
    result.color = value.color;
    result.matrix = value.matrix;
    return result;
}

std::string roleFor(SourceNodeId id, const BoundIds& ids) {
    if (id == ids.root) return "root";
    if (id == ids.layer) return "layer";
    if (id == ids.group) return "group";
    if (id == ids.ellipse) return "ellipse";
    if (id == ids.fill) return "fill";
    return {};
}

std::string roleFor(PropertyId id, const BoundIds& ids) {
    if (id == ids.layerTransform) return "layer.transform";
    if (id == ids.layerOpacity) return "layer.opacity";
    if (id == ids.groupTransform) return "group.transform";
    if (id == ids.groupOpacity) return "group.opacity";
    if (id == ids.position) return "ellipse.position";
    if (id == ids.size) return "ellipse.size";
    if (id == ids.color) return "fill.color";
    if (id == ids.fillOpacity) return "fill.opacity";
    return {};
}

const MotionPropertyRecord& propertyFor(const MotionAssetModel& model, PropertyId id) {
    const auto* property = model.property(id);
    require(property && property->present, "bound property present");
    return *property;
}

const MotionSourceNodeRecord& sourceFor(const MotionAssetModel& model, SourceNodeId id) {
    const auto* source = model.sourceNode(id);
    require(source && source->present, "bound source present");
    return *source;
}

SourceRoleSnapshot sourceRole(const MotionAssetModel& model, SourceNodeId id, const BoundIds& ids) {
    const auto& source = sourceFor(model, id);
    SourceRoleSnapshot role;
    role.role = roleFor(id, ids);
    role.kind = source.kind;
    role.layerKind = source.layerKind;
    role.parentRole = roleFor(source.parent, ids);
    role.transformParentRole = roleFor(source.transformParent, ids);
    role.referencesComposition = source.referencedComposition.valid();
    role.debugName = source.debugName;
    role.hidden = source.hidden;
    role.enabled = source.enabled;
    role.authoredStatic = source.authoredStatic;
    role.autoOrient = source.autoOrient;
    role.authoredLayerId = source.authoredLayerId;
    role.authoredParentLayerId = source.authoredParentLayerId;
    role.inFrame = source.inFrame;
    role.outFrame = source.outFrame;
    role.startFrame = source.startFrame;
    role.timeStretch = source.timeStretch;
    role.dependencyBits = source.dependencyBits;
    role.fillRule = source.fillRule;
    role.strokeCap = source.strokeCap;
    role.strokeJoin = source.strokeJoin;
    role.gradientType = source.gradientType;
    role.maskMode = source.maskMode;
    role.matteMode = source.matteMode;
    role.blendMode = source.blendMode;
    role.pathDirection = source.pathDirection;
    role.polystarType = source.polystarType;
    role.trimMode = source.trimMode;
    role.miterLimit = source.miterLimit;
    role.repeaterMaximumCopies = source.repeaterMaximumCopies;
    role.gradientColorPointCount = source.gradientColorPointCount;
    role.layerWidth = source.layerWidth;
    role.layerHeight = source.layerHeight;
    role.solidColor = source.solidColor;
    role.hasSourceAssetReference = source.sourceAssetRefHash != 0;
    role.maskInverted = source.maskInverted;
    for (std::uint32_t offset = 0; offset < source.children.count; ++offset) {
        const auto index = source.children.first + offset;
        require(index < model.sourceChildIds.size(), "source child edge in range");
        role.childRoles.push_back(roleFor(model.sourceChildIds[index], ids));
    }
    return role;
}

PropertyRoleSnapshot propertyRole(const MotionAssetModel& model, PropertyId id, const BoundIds& ids) {
    const auto& property = propertyFor(model, id);
    PropertyRoleSnapshot role;
    role.role = roleFor(id, ids);
    role.ownerRole = roleFor(property.owner, ids);
    role.semantic = property.semantic;
    role.semanticIndex = property.semanticIndex;
    role.valueType = property.valueType;
    role.flags = property.flags;
    role.hasStaticValue = property.staticValue.valid();
    if (role.hasStaticValue) role.staticValue = valueFromRef(model, property.staticValue);
    if (property.track.valid()) {
        const auto* track = model.track(property.track);
        require(track && track->present && track->segments.count == 1, "bound track has one segment");
        const auto segmentIndex = track->segments.first;
        require(segmentIndex < model.segments.size(), "bound segment edge in range");
        const auto* segment = model.segment(model.segments[segmentIndex].id);
        require(segment && segment->present, "bound segment present");
        role.segment.present = true;
        role.segment.firstFrame = segment->firstFrame;
        role.segment.endFrame = segment->endFrame;
        role.segment.interpolation = segment->interpolation;
        role.segment.spatialInterpolation = segment->spatialInterpolation;
        role.segment.start = valueFromRef(model, segment->startValue);
        role.segment.end = valueFromRef(model, segment->endValue);
        role.segment.temporalControl1 = segment->temporalControl1;
        role.segment.temporalControl2 = segment->temporalControl2;
    }
    return role;
}

PathSnapshot pathSnapshot(const runtime::CanonicalGeometry& geometry) {
    return {geometry.fillRule, geometry.path.verbs, geometry.path.points, geometry.path.controlBounds};
}

PaintSnapshot paintSnapshot(const runtime::CanonicalPaint& paint) {
    return {paint.stroke.enabled, paint.stroke.width, paint.stroke.cap, paint.stroke.join,
        paint.paint.kind, paint.paint.solid};
}

EvaluationSample sampleAt(const BoundIds& ids,
                          evaluation::PropertyEvaluator& evaluator,
                          evaluation::PropertyEvaluationWorkspace& workspace,
                          double frame) {
    const auto view = evaluator.evaluate(frame, workspace);
    require(static_cast<bool>(view), "semantic evaluator sample succeeds");
    auto findProperty = [&](PropertyId id) -> const evaluation::EvaluatedProperty& {
        const auto found = std::find_if(view.properties.begin(), view.properties.end(),
            [&](const auto& property) { return property.id == id; });
        require(found != view.properties.end(), "evaluated property role present");
        return *found;
    };
    auto findTransform = [&](SourceNodeId id) -> const evaluation::EvaluatedNodeTransform& {
        const auto found = std::find_if(view.nodeTransforms.begin(), view.nodeTransforms.end(),
            [&](const auto& transform) { return transform.node == id; });
        require(found != view.nodeTransforms.end(), "evaluated transform role present");
        return *found;
    };
    const auto& position = findProperty(ids.position);
    const auto& layer = findTransform(ids.layer);
    const auto& group = findTransform(ids.group);
    return {frame, valueFromEvaluated(position.value),
        layer.localMatrix, layer.worldMatrix, group.localMatrix, group.worldMatrix,
        layer.localOpacity, layer.worldOpacity, group.localOpacity, group.worldOpacity};
}

SemanticSnapshot buildSnapshot(const std::shared_ptr<const MotionAssetModel>& model,
                               const BoundIds& ids,
                               const std::vector<double>& frames) {
    evaluation::PropertyEvaluator evaluator{model};
    evaluation::PropertyEvaluationWorkspace workspace;
    evaluator.prepare(workspace);
    SemanticSnapshot snapshot;
    snapshot.width = model->logicalWidth;
    snapshot.height = model->logicalHeight;
    snapshot.totalFrames = model->totalFrames;
    snapshot.frameRate = model->frameRate;
    for (const auto id : {ids.root, ids.layer, ids.group, ids.ellipse, ids.fill})
        snapshot.sources.push_back(sourceRole(*model, id, ids));
    for (const auto id : {ids.layerTransform, ids.layerOpacity, ids.groupTransform, ids.groupOpacity,
             ids.position, ids.size, ids.color, ids.fillOpacity})
        snapshot.properties.push_back(propertyRole(*model, id, ids));
    const auto* node = model->node(ids.renderNode);
    const auto* geometry = model->geometry(ids.geometry);
    const auto* paint = model->paint(ids.paint);
    require(node && node->present && geometry && geometry->present && paint && paint->present,
        "semantic render resources present");
    snapshot.resources.renderDependencyBits = node->dependencyBits;
    snapshot.resources.geometryClass = geometry->resourceClass;
    snapshot.resources.hasStaticGeometry = geometry->staticValue.has_value();
    if (geometry->staticValue) snapshot.resources.geometry = pathSnapshot(*geometry->staticValue);
    snapshot.resources.paintClass = paint->resourceClass;
    snapshot.resources.hasStaticPaint = paint->staticValue.has_value();
    if (paint->staticValue) snapshot.resources.paint = paintSnapshot(*paint->staticValue);
    for (const auto frame : frames) {
        snapshot.samples.push_back(sampleAt(ids, evaluator, workspace, frame));
        ++counters.evaluatorSamples;
    }
    return snapshot;
}

std::string compareSnapshots(const SemanticSnapshot& left, const SemanticSnapshot& right) {
    if (left.width != right.width || left.height != right.height
        || left.totalFrames != right.totalFrames || left.frameRate != right.frameRate)
        return "composition metadata";
    if (left.sources != right.sources) return "semantic source roles/edges/defaults";
    if (left.properties != right.properties) return "semantic property values/tracks/timing";
    if (!(left.resources == right.resources)) return "geometry/paint resource semantics";
    if (left.samples != right.samples) return "evaluated transforms/opacities/properties";
    return {};
}

bool semanticSnapshotsEqual(const SemanticSnapshot& left, const SemanticSnapshot& right) {
    return compareSnapshots(left, right).empty();
}

BoundIds ownIds(const render::detail::OwnNativeEllipsePreparedAsset& prepared);
std::shared_ptr<const render::detail::OwnNativeEllipsePreparedAsset> ownPreparedFrom(
    std::string_view source);

void equalSemanticSnapshotsCompare() {
    const SemanticSnapshot left{512, 512, 61, 60.0,
        {{"root"}, {"layer"}, {"group"}, {"ellipse"}, {"fill"}},
        {{"ellipse.position"}}, {}, {{0.0}}};
    const SemanticSnapshot right = left;
    require(semanticSnapshotsEqual(left, right), "equal semantic snapshots compare");
}

void comparatorMutationWitnesses() {
    SemanticSnapshot base{512, 512, 61, 60.0,
        {{"root", SourceNodeKind::Composition, SourceLayerKind::None, {}, {}, false, {}, {}, false, true},
            {"layer", SourceNodeKind::Layer, SourceLayerKind::Shape, "root", {}, false, {}, {}, false, true},
            {"group", SourceNodeKind::ShapeGroup, SourceLayerKind::None, "layer", {}, false, {}, {}, false, true},
            {"ellipse", SourceNodeKind::Ellipse, SourceLayerKind::None, "group", {}, false, {}, {}, false, true},
            {"fill", SourceNodeKind::Fill, SourceLayerKind::None, "group", {}, false, {}, {}, false, true}},
        {{"ellipse.position", "ellipse", PropertySemantic::EllipsePosition, 0,
            PropertyValueType::Vec2, PropertyFlagAnimated, false, {},
            {true, 0.0, 60.0, SegmentInterpolation::Linear, SpatialInterpolation::None,
                {PropertyValueType::Vec2, 0.0F, {-76, 0}},
                {PropertyValueType::Vec2, 0.0F, {76, 0}}, {0, 0}, {1, 1}}}},
        {ResourceClass::AssetStatic, true,
            {runtime::FillRule::Winding, {runtime::PathVerb::MoveTo}, {{0, 0}}, {true, 0, 0, 0, 0}},
            ResourceClass::AssetStatic, true,
            {false, 0.0F, runtime::LineCap::Flat, runtime::LineJoin::Miter,
                runtime::PaintKind::Solid, {127, 25, 254, 255}},
            StaticDependencyTransform},
        {{30.0, {PropertyValueType::Vec2, 0.0F, {0, 0}}}}};
    const auto witness = [&](auto mutate, std::string_view label) {
        auto changed = base;
        mutate(changed);
        require(!semanticSnapshotsEqual(base, changed), label);
        ++counters.comparatorWitnesses;
    };
    witness([](auto& snapshot) { snapshot.properties[0].segment.end.vec2.x = 77; },
        "comparator sees value");
    witness([](auto& snapshot) { snapshot.sources[3].kind = SourceNodeKind::Rectangle; },
        "comparator sees semantic role");
    witness([](auto& snapshot) { snapshot.sources[1].enabled = false; },
        "comparator sees enabled bit");
    witness([](auto& snapshot) { snapshot.properties[0].segment.temporalControl2.x = 0.5F; },
        "comparator sees keyframe control");
    witness([](auto& snapshot) { snapshot.resources.geometry.points[0].x = 1; },
        "comparator sees path point");
    witness([](auto& snapshot) { snapshot.resources.paint.solid.r = 128; },
        "comparator sees paint byte");
    witness([](auto& snapshot) { snapshot.resources.geometryClass = ResourceClass::InstanceEvaluated; },
        "comparator sees resource class");
}

void sourceDefaultComparatorWitnesses() {
    const auto prepared = ownPreparedFrom(staticOriginUnitSource());
    const auto ids = ownIds(*prepared);
    const auto baseline = buildSnapshot(prepared->model, ids, {0.0});
    const auto witness = [&](auto mutate, std::string_view label) {
        auto changed = std::make_shared<MotionAssetModel>(*prepared->model);
        mutate(*changed);
        const auto changedSnapshot = buildSnapshot(changed, ids, {0.0});
        require(!semanticSnapshotsEqual(baseline, changedSnapshot), label);
        ++counters.comparatorWitnesses;
    };
    witness([&](auto& model) { model.sourceNodes[ids.layer.index()].transformParent = ids.root; },
        "comparator sees source transform-parent default");
    witness([&](auto& model) { model.sourceNodes[ids.layer.index()].referencedComposition = makeId<CompositionId>(0); },
        "comparator sees source referenced-composition default");
    witness([&](auto& model) { model.sourceNodes[ids.layer.index()].autoOrient = true; },
        "comparator sees source auto-orient default");
    witness([&](auto& model) { model.sourceNodes[ids.layer.index()].authoredParentLayerId = 7; },
        "comparator sees source authored-parent default");
    witness([&](auto& model) { model.sourceNodes[ids.layer.index()].startFrame = 1.0; },
        "comparator sees source start-frame default");
    witness([&](auto& model) { model.sourceNodes[ids.layer.index()].maskInverted = true; },
        "comparator sees source mask-inverted default");
    witness([&](auto& model) { model.sourceNodes[ids.layer.index()].repeaterMaximumCopies = 1.0F; },
        "comparator sees source repeater default");
    witness([&](auto& model) { model.sourceNodes[ids.layer.index()].layerWidth = 512; },
        "comparator sees source layer-width default");
    witness([&](auto& model) { model.sourceNodes[ids.layer.index()].layerHeight = 512; },
        "comparator sees source layer-height default");
    witness([&](auto& model) { model.sourceNodes[ids.layer.index()].sourceAssetRefHash = 1; },
        "comparator sees source asset-reference absence default");
}

BoundIds ownIds(const render::detail::OwnNativeEllipsePreparedAsset& prepared) {
    const auto& binding = prepared.authored->binding;
    return {binding.root, binding.layer, binding.group, binding.ellipse, binding.fill,
        binding.layerTransform, binding.layerOpacity, binding.groupTransform, binding.groupOpacity,
        binding.position, binding.size, binding.color, binding.fillOpacity,
        makeId<LayerId>(0), makeId<LayerId>(1), makeId<NodeId>(0), makeId<GeometryId>(0), makeId<PaintId>(0)};
}

BoundIds oracleIds(const runtime::detail::NativeEllipseCertificate& certificate) {
    const auto& binding = certificate.slot.binding;
    return {binding.root, binding.layer, binding.group, binding.ellipse, binding.fill,
        binding.layerTransform, binding.layerOpacity, binding.groupTransform, binding.groupOpacity,
        binding.position, binding.size, binding.color, binding.fillOpacity,
        certificate.slot.root.id, certificate.slot.shape.id, certificate.slot.draw.node,
        certificate.slot.draw.geometry, certificate.slot.draw.paint};
}

std::shared_ptr<const render::detail::OwnNativeEllipsePreparedAsset> ownPreparedFrom(
    std::string_view source) {
    const auto parsed = formats::detail::readOwnJson(source);
    require(static_cast<bool>(parsed), "own semantic source parsed");
    const auto authored = runtime::detail::buildOwnNativeEllipseModel(*parsed.document);
    require(static_cast<bool>(authored), "own authored semantic model prepared");
    const auto prepared = render::detail::prepareOwnNativeEllipseAsset(authored.prepared);
    require(static_cast<bool>(prepared), "own semantic resources prepared");
    return prepared.prepared;
}

runtime::detail::NativeEllipseCertificateResult oraclePreparedFrom(runtime::Runtime& runtime,
                                                                   std::string_view label,
                                                                   std::string_view source) {
    auto oracle = runtime::detail::prepareNativeEllipseCertificate(runtime, source);
    if (!oracle) {
        std::ostringstream message;
        message << label << " oracle native ellipse certificate prepared code=" << static_cast<int>(oracle.code)
                << " admission=" << static_cast<int>(oracle.admission.code)
                << " binding=" << static_cast<int>(oracle.bindingCode)
                << " scan=" << static_cast<int>(oracle.scanCode)
                << " bytes=" << source.size()
                << " prefix=" << std::string{source.substr(0, std::min<std::size_t>(source.size(), 16))}
                << " reference=" << oracle.referenceError.message;
        require(false, message.str());
    }
    require(oracle.certificate && oracle.certificate->model && oracle.certificate->input,
        "oracle semantic owners present");
    return oracle;
}

void compareCommonCase(std::string_view label, const std::string& source) {
    runtime::Runtime runtime;
    auto oracle = oraclePreparedFrom(runtime, label, source);
    auto own = ownPreparedFrom(source);
    const std::vector<double> frames{-1.0, 0.0, 0.5, 10.0, 30.0, 60.0, 61.0, 30.0, 1.0, 0.0};
    const auto ownSnapshot = buildSnapshot(own->model, ownIds(*own), frames);
    const auto oracleSnapshot = buildSnapshot(oracle.certificate->model, oracleIds(*oracle.certificate), frames);
    const auto mismatch = compareSnapshots(ownSnapshot, oracleSnapshot);
    require(mismatch.empty(), std::string{label} + " semantic snapshots compare: " + mismatch);
    ++counters.commonCases;
}

void literalAnchorChecks() {
    {
        auto own = ownPreparedFrom(test::ellipseFixture());
        const auto snapshot = buildSnapshot(own->model, ownIds(*own), {0.0, 60.0});
        require(snapshot.samples[0].position.vec2 == MotionVec2Value{-76, 0}
            && snapshot.samples[1].position.vec2 == MotionVec2Value{76, 0},
            "baseline endpoint anchors");
        require(snapshot.resources.geometryClass == ResourceClass::InstanceEvaluated
            && !snapshot.resources.hasStaticGeometry
            && snapshot.resources.renderDependencyBits == (StaticDependencyTransform | StaticDependencyGeometry),
            "animated geometry remains evaluated with geometry dependency");
    }
    {
        auto own = ownPreparedFrom(linearSource());
        const auto snapshot = buildSnapshot(own->model, ownIds(*own), {30.0});
        require(snapshot.samples[0].position.vec2 == MotionVec2Value{0, 0},
            "linear midpoint anchor");
    }
    {
        auto own = ownPreparedFrom(staticOriginUnitSource());
        const auto snapshot = buildSnapshot(own->model, ownIds(*own), {0.0});
        constexpr float k = 0.5522847498F;
        const std::array<runtime::Vec2, 13> points{{{0, -1}, {k, -1}, {1, -k}, {1, 0},
            {1, k}, {k, 1}, {0, 1}, {-k, 1}, {-1, k}, {-1, 0}, {-1, -k}, {-k, -1}, {0, -1}}};
        require(snapshot.resources.geometryClass == ResourceClass::AssetStatic
            && snapshot.resources.hasStaticGeometry
            && snapshot.resources.geometry.verbs.size() == 6
            && snapshot.resources.geometry.points.size() == points.size(),
            "unit static path shape");
        for (std::size_t index = 0; index < points.size(); ++index) {
            require(snapshot.resources.geometry.points[index].x == points[index].x
                && snapshot.resources.geometry.points[index].y == points[index].y,
                "unit static path point anchor");
        }
        require(snapshot.resources.paint.solid.r == 127 && snapshot.resources.paint.solid.g == 25
            && snapshot.resources.paint.solid.b == 254 && snapshot.resources.paint.solid.a == 255,
            "unit static paint byte anchor");
    }
}

void commonDomainMatrix() {
    compareCommonCase("baseline", test::ellipseFixture());
    compareCommonCase("static boundary", test::staticEllipseFixture(test::ellipseFixture()));
    compareCommonCase("static origin unit", staticOriginUnitSource());
    compareCommonCase("linear controls", linearSource());
    compareCommonCase("active10..20", activeSource());
    compareCommonCase("translated layer", replace(test::ellipseFixture(), "[256, 256, 0]", "[11, 22, 0]"));
    compareCommonCase("fractional59.94", replace(test::ellipseFixture(), "\"fr\": 60", "\"fr\": 59.94"));
    compareCommonCase("equivalent rate", replace(test::ellipseFixture(), "\"fr\": 60", "\"fr\": 6000e-2"));
    auto changed = replace(test::ellipseFixture(), "[120, 120]", "[80, 140]");
    changed = replace(std::move(changed), "[0.08, 0.72, 0.95, 1]", "[0.25, 0.5, 1, 1]");
    compareCommonCase("changed size/color", changed);
    compareCommonCase("all names absent", absentNamesSource());
    compareCommonCase("all names empty", emptyNamesSource());
    compareCommonCase("UTF8 snowman", replace(test::ellipseFixture(), "\"Moving Circle\"", "\"\\u2603\""));
    literalAnchorChecks();
}

void policyRows() {
    {
        const auto ipTiny = replace(test::ellipseFixture(), "  \"ip\": 0,\n  \"op\": 61",
            "  \"ip\": 0e999999999999999999999999,\n  \"op\": 61");
        const auto own = ownPreparedFrom(ipTiny);
        require(own->authored->input->layerInFrame == 0 && own->model->totalFrames == 61,
            "own accepts enormous lexical zero ip as baseline");
        runtime::Runtime runtime;
        const auto old = runtime::detail::prepareNativeEllipseCertificate(runtime, ipTiny);
        require(!old && old.code == runtime::detail::NativeEllipseCertificateCode::AdmissionRejected,
            "old certificate rejects enormous lexical zero ip");
        ++counters.policyRows;
    }
    {
        const auto lowSurrogate = replace(test::ellipseFixture(), "Moving Circle", "\\uDC00");
        const auto ownRead = formats::detail::readOwnJson(lowSurrogate);
        require(!ownRead && ownRead.code == formats::detail::OwnJsonReadCode::InvalidJson
            && ownRead.path == "/", "own reader rejects escaped low surrogate before model");
        runtime::Runtime runtime;
        const auto old = runtime::detail::prepareNativeEllipseCertificate(runtime, lowSurrogate);
        require(old.code == runtime::detail::NativeEllipseCertificateCode::Certified
            || old.code == runtime::detail::NativeEllipseCertificateCode::AdmissionRejected
            || old.code == runtime::detail::NativeEllipseCertificateCode::BindingRejected
            || old.code == runtime::detail::NativeEllipseCertificateCode::ScanRejected
            || old.code == runtime::detail::NativeEllipseCertificateCode::ReferenceError,
            "old low-surrogate certificate bounded outcome recorded");
        std::cout << "low-surrogate legacy certificate code=" << static_cast<int>(old.code)
                  << " admission=" << static_cast<int>(old.admission.code)
                  << " binding=" << static_cast<int>(old.bindingCode)
                  << " scan=" << static_cast<int>(old.scanCode) << '\n';
        ++counters.policyRows;
    }
    {
        const auto tinyFr = replace(test::ellipseFixture(), "\"fr\": 60", "\"fr\": 1e-9999");
        const auto parsed = formats::detail::readOwnJson(tinyFr);
        require(static_cast<bool>(parsed), "tiny fr source parsed");
        const auto own = runtime::detail::buildOwnNativeEllipseModel(*parsed.document);
        require(!own && !own.prepared
            && own.code == runtime::detail::OwnNativeEllipseModelCode::UnsupportedNumericConversion
            && own.admission.accepted(), "tiny fr has own numeric conversion failure without preparation");
        ++counters.policyRows;
    }
}

void ownershipIsolationRows() {
    std::shared_ptr<const render::detail::OwnNativeEllipsePreparedAsset> retained;
    std::shared_ptr<const MotionAssetModel> retainedModel;
    std::shared_ptr<const runtime::CanonicalGeometry> retainedGeometry;
    std::shared_ptr<const runtime::CanonicalPaint> retainedPaint;
    {
        retained = ownPreparedFrom(staticOriginUnitSource());
        retainedModel = retained->model;
        runtime::Runtime runtime;
        auto oracle = oraclePreparedFrom(runtime, "lifetime static origin unit", staticOriginUnitSource());
        require(oracle.certificate->model != retained->model, "oracle and own models distinct while alive");
        runtime::EvaluatedScene scene;
        scene.sourceAssetHash = test::ownRawHash(staticOriginUnitSource());
        scene.drawItems.resize(1);
        auto& item = scene.drawItems[0];
        item.modelNode = makeId<NodeId>(0);
        item.modelGeometry = makeId<GeometryId>(0);
        item.modelPaint = makeId<PaintId>(0);
        item.localGeometryAvailable = true;
        item.localPaintAvailable = true;
        require(static_cast<bool>(detail::applyAssetModel(retained->model, scene)),
            "own model applies before oracle destruction");
        retainedGeometry = item.canonicalGeometry;
        retainedPaint = item.canonicalPaint;
    }
    evaluation::PropertyEvaluator evaluator{retainedModel};
    evaluation::PropertyEvaluationWorkspace workspace;
    evaluator.prepare(workspace);
    const auto view = evaluator.evaluate(0.0, workspace);
    require(static_cast<bool>(view), "own evaluator survives oracle/runtime destruction");
    require(retainedGeometry && retainedPaint && retainedGeometry->path.points.size() == 13
        && retainedPaint->paint.solid.r == 127, "own resource aliases survive oracle/runtime destruction");

    const auto a = ownPreparedFrom(staticOriginUnitSource());
    auto changed = replace(staticOriginUnitSource(), "[0.5, 0.1, 0.999, 1]", "[1, 0, 1, 1]");
    changed = replace(std::move(changed), "[256, 256, 0]", "[33, 44, 0]");
    const auto b = ownPreparedFrom(changed);
    require(a->authored->exactJson != b->authored->exactJson && a->model.get() != b->model.get()
        && a->model->paints[0].staticValue->paint.solid.r == 127
        && b->model->paints[0].staticValue->paint.solid.r == 255
        && a->model->matrixValues[0].dx == 256 && b->model->matrixValues[0].dx == 33,
        "two owned sources keep distinct bytes/model pointers/values");
}

} // namespace

int main() {
    try {
        equalSemanticSnapshotsCompare();
        comparatorMutationWitnesses();
        sourceDefaultComparatorWitnesses();
        commonDomainMatrix();
        policyRows();
        ownershipIsolationRows();
        std::cout << "own native ellipse model differential tests passed\n";
        std::cout << "counters cases=" << counters.commonCases
                  << " samples=" << counters.evaluatorSamples
                  << " assertions=" << counters.assertions
                  << " witnesses=" << counters.comparatorWitnesses
                  << " policyRows=" << counters.policyRows << '\n';
        std::cout << "comparison exclusions: raw source/render IDs, revision, source/parsed/topology/resource/full fingerprints, Asset handles, and arbitrary render display labels\n";
        return EXIT_SUCCESS;
    } catch (const std::exception& error) {
        std::cerr << "FAILED: " << error.what() << '\n';
        return EXIT_FAILURE;
    }
}
