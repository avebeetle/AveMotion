#include "OwnPrimitiveBinding.hpp"
#include "OwnPrimitiveNumeric.hpp"
#include "NativeEllipseBindingHelpers.hpp"

#include <algorithm>
#include <type_traits>

namespace avemotion::runtime::detail {
namespace {
using namespace model;
using namespace binding_helpers;
using Code = NativeEllipseBindingCode;

bool all(const std::vector<bool>& rows) {
    return std::all_of(rows.begin(), rows.end(), [](bool used) { return used; });
}
bool claim(std::vector<bool>& rows, std::size_t index) {
    if (index >= rows.size() || rows[index]) return false;
    rows[index] = true;
    return true;
}
bool animated(const OwnPrimitiveGroupValues& v) {
    return v.position.animated || v.size.animated || (v.roundness && v.roundness->animated);
}

// A single validation walk owns consumption of every authored table row. Render
// resources may be attached later, but cannot affect these semantic bindings.
class PrimitiveBinder final {
public:
    explicit PrimitiveBinder(const MotionAssetModel& value) : m(value),
        nodes(m.sourceNodes.size()), edges(m.sourceChildIds.size()),
        propertyEdges(m.sourcePropertyIds.size()), properties(m.properties.size()),
        tracks(m.tracks.size()), segments(m.segments.size()), scalars(m.scalarValues.size()),
        vectors(m.vec2Values.size()), colors(m.colorValues.size()), matrices(m.matrixValues.size()) {}

    bool check(bool ok, Code failure) {
        if (!ok && code == Code::Bound) code = failure;
        return ok;
    }
    const MotionSourceNodeRecord* node(SourceNodeId id, SourceNodeId parent,
        SourceNodeKind kind, std::size_t childCount, std::size_t propertyCount,
        const std::string& name, bool moving,
        SourcePathDirection direction = SourcePathDirection::Clockwise) {
        if (!check(id.valid() && claim(nodes,id.index()),Code::TopologyMismatch)) return nullptr;
        const auto& n = m.sourceNodes[id.index()];
        if (!check(n.parent == parent && n.children.count == childCount
                && n.properties.count == propertyCount,Code::TopologyMismatch)
            || !check(n.kind == kind && defaultNode(n,moving,direction)
                && n.debugName == name && n.nameHash == nameHash(name),Code::SourceIdentityMismatch)) return nullptr;
        for (std::size_t i=0; i<n.children.count; ++i) {
            const auto e = static_cast<std::size_t>(n.children.first)+i;
            const auto c = m.sourceChildIds[e];
            if (!check(claim(edges,e) && c.valid() && c.index()<m.sourceNodes.size()
                && m.sourceNodes[c.index()].parent==id,Code::TopologyMismatch)) return nullptr;
        }
        if (kind != SourceNodeKind::Layer && !check(n.layerKind == SourceLayerKind::None
            && n.authoredLayerId == -1 && n.inFrame == 0 && n.outFrame == 0,
            Code::SourceIdentityMismatch)) return nullptr;
        return &n;
    }
    PropertyId property(const MotionSourceNodeRecord& n, PropertySemantic semantic,
                        PropertyValueType type, bool moving) {
        PropertyId found;
        for (std::size_t i=0; i<n.properties.count; ++i) {
            const auto edge = static_cast<std::size_t>(n.properties.first)+i;
            const auto id = m.sourcePropertyIds[edge];
            const auto* p = m.property(id);
            if (!check(p && p->present && p->id == id && p->owner == n.id
                    && p->semanticIndex == 0,Code::PropertyShapeMismatch)) return {};
            if (p->semantic != semantic) continue;
            if (!check(!found.valid() && claim(propertyEdges,edge) && claim(properties,id.index())
                    && p->valueType == type,Code::PropertyShapeMismatch)) return {};
            if (!check(moving ? p->flags == PropertyFlagAnimated && !p->staticValue.valid()
                    && p->track.valid() : staticValue(m,*p,type),Code::PropertyShapeMismatch)) return {};
            found = id;
        }
        check(found.valid(),Code::PropertyShapeMismatch);
        return found;
    }
    bool value(MotionValueRef ref, float expected) {
        return check(valueRef(m,ref,PropertyValueType::Scalar) && claim(scalars,ref.index),Code::PropertyShapeMismatch)
            && check(std::isfinite(m.scalarValues[ref.index]) && m.scalarValues[ref.index]==expected,Code::ValueMismatch);
    }
    bool value(MotionValueRef ref, MotionVec2Value expected) {
        return check(valueRef(m,ref,PropertyValueType::Vec2) && claim(vectors,ref.index),Code::PropertyShapeMismatch)
            && check(finite(m.vec2Values[ref.index]) && m.vec2Values[ref.index]==expected,Code::ValueMismatch);
    }
    bool value(MotionValueRef ref, MotionColorValue expected) {
        return check(valueRef(m,ref,PropertyValueType::Color) && claim(colors,ref.index),Code::PropertyShapeMismatch)
            && check(finite(m.colorValues[ref.index]) && m.colorValues[ref.index]==expected,Code::ValueMismatch);
    }
    bool value(MotionValueRef ref, MotionMatrix3x2Value expected) {
        return check(valueRef(m,ref,PropertyValueType::Matrix3x2) && claim(matrices,ref.index),Code::PropertyShapeMismatch)
            && check(finite(m.matrixValues[ref.index]) && m.matrixValues[ref.index]==expected,Code::ValueMismatch);
    }
    template<class T> PropertyId fixed(const MotionSourceNodeRecord& n,
        PropertySemantic semantic, PropertyValueType type, T expected) {
        const auto id = property(n,semantic,type,false);
        if (id.valid()) value(m.properties[id.index()].staticValue,expected);
        return id;
    }
    template<class T> PropertyId varying(const MotionSourceNodeRecord& n,
        PropertySemantic semantic, PropertyValueType type, const T& expected) {
        const auto id = property(n,semantic,type,expected.animated);
        if (!id.valid()) return {};
        const auto& p = m.properties[id.index()];
        if (!expected.animated) { value(p.staticValue,expected.start); return id; }
        if (!check(claim(tracks,p.track.index()),Code::TrackMismatch)) return {};
        const auto& t = m.tracks[p.track.index()];
        if (!check(t.present && t.id==p.track && t.property==id
            && t.firstFrame==expected.firstFrame && t.endFrame==expected.lastFrame
            && range(t.segments,m.segments.size()) && t.segments.count==1
            && claim(segments,t.segments.first),Code::TrackMismatch)) return {};
        const auto& s = m.segments[t.segments.first];
        const bool linear = expected.outgoing==MotionVec2Value{0,0} && expected.incoming==MotionVec2Value{1,1};
        if (!check(s.present && s.id==makeId<SegmentId>(t.segments.first) && s.track==t.id
            && s.firstFrame==expected.firstFrame && s.endFrame==expected.lastFrame
            && s.interpolation==(linear?SegmentInterpolation::Linear:SegmentInterpolation::CubicBezier)
            && s.spatialInterpolation==SpatialInterpolation::None
            && s.temporalControl1==expected.outgoing && s.temporalControl2==expected.incoming
            && s.spatialInTangent==MotionVec2Value{} && s.spatialOutTangent==MotionVec2Value{},
            Code::TrackMismatch)) return {};
        value(s.startValue,expected.start); value(s.endValue,expected.end);
        return id;
    }
    bool exhausted() {
        return check(all(nodes)&&all(edges),Code::TopologyMismatch)
            && check(all(propertyEdges)&&all(properties)&&all(scalars)&&all(vectors)
                &&all(colors)&&all(matrices),Code::PropertyShapeMismatch)
            && check(all(tracks)&&all(segments),Code::TrackMismatch);
    }
    Code code = Code::Bound;
private:
    const MotionAssetModel& m;
    std::vector<bool> nodes,edges,propertyEdges,properties,tracks,segments,scalars,vectors,colors,matrices;
};
} // namespace

OwnPrimitiveBindingResult bindOwnPrimitiveModel(const OwnPrimitiveInput& input, const MotionAssetModel& m) {
    const auto fail = [](Code code) { return OwnPrimitiveBindingResult{code,std::nullopt}; };
    const auto numeric = interpretOwnPrimitiveInput(input);
    if (!numeric) return fail(Code::UnsupportedNumericConversion);
    if (input.groups.empty() || input.groups.size()>16 || numeric->groups.size()!=input.groups.size())
        return fail(Code::InvalidModelTable);
    const auto& v = *numeric;
    std::size_t propertyCount=2, scalarCount=1, vectorCount=0, animatedCount=0;
    for (std::size_t g=0; g<input.groups.size(); ++g) {
        const auto& group = v.groups[g];
        const bool rectangle = input.groups[g].kind==OwnPrimitiveKind::Rectangle;
        if (rectangle!=group.roundness.has_value()) return fail(Code::InvalidModelTable);
        propertyCount+=6+(rectangle?1:0);
        scalarCount+=2+(rectangle?(group.roundness->animated?2:1):0);
        vectorCount+=2+(group.position.animated?1:0)+(group.size.animated?1:0);
        animatedCount+=(group.position.animated?1:0)+(group.size.animated?1:0)
            +(rectangle&&group.roundness->animated?1:0);
    }
    const auto n=input.groups.size();
    const auto& stats=m.statistics;
    if (m.schemaVersion!=MotionAssetModel::kSchemaVersion || !stats.directParsedModel
        || m.compositions.size()!=1 || m.sourceNodes.size()!=2+3*n || m.sourceChildIds.size()!=1+3*n
        || m.properties.size()!=propertyCount || m.sourcePropertyIds.size()!=propertyCount
        || m.tracks.size()!=animatedCount || m.segments.size()!=animatedCount
        || m.scalarValues.size()!=scalarCount || m.vec2Values.size()!=vectorCount
        || m.colorValues.size()!=n || m.matrixValues.size()!=n+1
        || !m.shapeValues.empty() || !m.shapePoints.empty() || !m.gradientValues.empty() || !m.gradientFloats.empty()
        || stats.compositionCount!=1 || stats.sourceNodeCount!=m.sourceNodes.size()
        || stats.propertyCount!=propertyCount || stats.staticPropertyCount!=propertyCount-animatedCount
        || stats.animatedPropertyCount!=animatedCount || stats.trackCount!=animatedCount
        || stats.segmentCount!=animatedCount || stats.scalarValueCount!=scalarCount
        || stats.vec2ValueCount!=vectorCount || stats.colorValueCount!=n || stats.matrixValueCount!=n+1
        || stats.shapeValueCount!=0 || stats.gradientValueCount!=0) return fail(Code::InvalidModelTable);
    const auto& comp=m.compositions.front();
    if (!comp.present || comp.id!=makeId<CompositionId>(0) || !comp.rootNode.valid()
        || comp.debugName!="root" || comp.logicalWidth!=input.width || comp.logicalHeight!=input.height
        || comp.firstFrame!=0 || comp.endFrame!=input.endFrame || comp.frameRate!=static_cast<double>(v.frameRate)
        || m.logicalWidth!=input.width || m.logicalHeight!=input.height || m.totalFrames!=input.endFrame
        || m.frameRate!=static_cast<double>(v.frameRate)) return fail(Code::CompositionMismatch);
    for (std::size_t i=0; i<m.sourceNodes.size(); ++i) {
        const auto& node=m.sourceNodes[i];
        if (!node.present || node.id!=makeId<SourceNodeId>(i) || node.composition!=comp.id
            || !range(node.children,m.sourceChildIds.size()) || !range(node.properties,m.sourcePropertyIds.size()))
            return fail(Code::InvalidModelTable);
    }
    PrimitiveBinder check(m);
    OwnPrimitiveBinding binding;
    binding.root=comp.rootNode;
    const bool moving=std::any_of(v.groups.begin(),v.groups.end(),animated);
    const auto* root=check.node(binding.root,{},SourceNodeKind::Composition,1,0,"root",moving);
    if (!root) return fail(check.code);
    binding.layer=m.sourceChildIds[root->children.first];
    const auto* layer=check.node(binding.layer,binding.root,SourceNodeKind::Layer,n,2,
        effective(input.layerName,"layer:"+std::to_string(input.layerId)),moving);
    if (!layer) return fail(check.code);
    if (layer->layerKind!=SourceLayerKind::Shape || layer->authoredLayerId!=input.layerId
        || layer->inFrame!=input.layerInFrame || layer->outFrame!=input.layerOutFrame)
        return fail(Code::SourceIdentityMismatch);
    binding.layerTransform=check.fixed(*layer,PropertySemantic::TransformMatrix,PropertyValueType::Matrix3x2,
        MotionMatrix3x2Value{1,0,0,1,v.translation.x,v.translation.y});
    binding.layerOpacity=check.fixed(*layer,PropertySemantic::TransformOpacity,PropertyValueType::Scalar,100.0F);
    for (std::size_t g=0; g<n; ++g) {
        const auto& in=input.groups[g]; const auto& expected=v.groups[g];
        const bool rectangle=in.kind==OwnPrimitiveKind::Rectangle;
        OwnPrimitiveGroupBinding b;
        b.group=m.sourceChildIds[layer->children.first+g];
        const auto* group=check.node(b.group,binding.layer,SourceNodeKind::ShapeGroup,2,2,
            effective(in.groupName,"group"),animated(expected));
        if (!group) return fail(check.code);
        b.primitive=m.sourceChildIds[group->children.first];
        b.fill=m.sourceChildIds[group->children.first+1];
        const auto* primitive=check.node(b.primitive,b.group,
            rectangle?SourceNodeKind::Rectangle:SourceNodeKind::Ellipse,0,rectangle?3:2,
            effective(in.primitiveName,"source-node"),animated(expected),in.direction);
        const auto* fill=check.node(b.fill,b.group,SourceNodeKind::Fill,0,2,effective(in.fillName,"source-node"),false);
        if (!primitive || !fill) return fail(check.code);
        b.groupTransform=check.fixed(*group,PropertySemantic::TransformMatrix,PropertyValueType::Matrix3x2,
            MotionMatrix3x2Value{1,0,0,1,0,0});
        b.groupOpacity=check.fixed(*group,PropertySemantic::TransformOpacity,PropertyValueType::Scalar,100.0F);
        b.position=check.varying(*primitive,rectangle?PropertySemantic::RectanglePosition:PropertySemantic::EllipsePosition,
            PropertyValueType::Vec2,expected.position);
        b.size=check.varying(*primitive,rectangle?PropertySemantic::RectangleSize:PropertySemantic::EllipseSize,
            PropertyValueType::Vec2,expected.size);
        if (rectangle) b.roundness=check.varying(*primitive,PropertySemantic::RectangleRoundness,
            PropertyValueType::Scalar,*expected.roundness);
        b.color=check.fixed(*fill,PropertySemantic::FillColor,PropertyValueType::Color,expected.color);
        b.fillOpacity=check.fixed(*fill,PropertySemantic::FillOpacity,PropertyValueType::Scalar,100.0F);
        binding.groups.push_back(b);
    }
    check.exhausted();
    if (check.code!=Code::Bound) return fail(check.code);
    return {Code::Bound,std::move(binding)};
}
} // namespace avemotion::runtime::detail
