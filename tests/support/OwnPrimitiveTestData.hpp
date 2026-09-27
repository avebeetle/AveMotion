#pragma once
#include "OwnPrimitiveBinding.hpp"

#include <filesystem>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <string_view>
#include <set>

namespace avemotion::test {

inline std::string primitiveFixture() {
    std::ifstream input{std::filesystem::path{AVEMOTION_FIXTURE_DIR} /
        "primitive_geometry.json", std::ios::binary};
    if (!input) throw std::runtime_error("cannot open primitive fixture");
    return {std::istreambuf_iterator<char>{input}, std::istreambuf_iterator<char>{}};
}

inline std::string replacePrimitiveOnce(std::string source, std::string_view from,
                                        std::string_view to) {
    const auto position = source.find(from);
    if (position == std::string::npos || source.find(from, position + from.size()) != std::string::npos)
        throw std::logic_error("primitive mutation fragment must be unique");
    source.replace(position, from.size(), to);
    return source;
}

// Independently walks semantic source roles; never calls the production own binder.
inline runtime::detail::OwnPrimitiveBinding primitiveTestRoles(const model::MotionAssetModel& m) {
    using namespace model;
    const auto check=[](bool ok) { if(!ok) throw std::runtime_error("independent primitive source-role graph"); };
    check(m.compositions.size()==1 && m.sourceNodes.size()==23 && m.sourceChildIds.size()==22
        && m.properties.size()==48 && m.sourcePropertyIds.size()==48 && m.tracks.size()==5
        && m.segments.size()==5 && m.statistics.staticPropertyCount==43
        && m.statistics.animatedPropertyCount==5);
    std::set<std::uint32_t> nodes,edges,propertyEdges,properties;
    const auto node=[&](SourceNodeId id,SourceNodeId parent,SourceNodeKind kind,
                        std::size_t children,std::size_t props) -> const MotionSourceNodeRecord& {
        check(id.valid() && id.index()<m.sourceNodes.size() && nodes.insert(id.index()).second);
        const auto& n=m.sourceNodes[id.index()];
        check(n.present && n.id==id && n.parent==parent && n.kind==kind && n.composition==m.compositions[0].id
            && n.children.count==children && n.properties.count==props
            && std::size_t(n.children.first)+n.children.count<=m.sourceChildIds.size()
            && std::size_t(n.properties.first)+n.properties.count<=m.sourcePropertyIds.size());
        for(std::uint32_t i=0;i<n.children.count;++i) check(edges.insert(n.children.first+i).second);
        return n;
    };
    const auto prop=[&](const MotionSourceNodeRecord& n,PropertySemantic semantic,PropertyValueType type) {
        PropertyId found;
        for(std::uint32_t i=0;i<n.properties.count;++i) {
            const auto id=m.sourcePropertyIds[n.properties.first+i];
            const auto* p=m.property(id);
            check(p && p->present && p->id==id && p->owner==n.id && p->semanticIndex==0);
            if(p->semantic==semantic) {
                check(!found.valid() && p->valueType==type && properties.insert(id.index()).second
                    && propertyEdges.insert(n.properties.first+i).second);
                found=id;
            }
        }
        check(found.valid()); return found;
    };
    runtime::detail::OwnPrimitiveBinding result;
    result.root=m.compositions[0].rootNode;
    const auto& root=node(result.root,{},SourceNodeKind::Composition,1,0);
    result.layer=m.sourceChildIds[root.children.first];
    const auto& layer=node(result.layer,result.root,SourceNodeKind::Layer,7,2);
    check(layer.layerKind==SourceLayerKind::Shape);
    result.layerTransform=prop(layer,PropertySemantic::TransformMatrix,PropertyValueType::Matrix3x2);
    result.layerOpacity=prop(layer,PropertySemantic::TransformOpacity,PropertyValueType::Scalar);
    const bool rectangles[]={true,true,true,false,false,true,false};
    const bool ccw[]={false,false,true,false,true,false,true};
    for(std::size_t g=0;g<7;++g) {
        runtime::detail::OwnPrimitiveGroupBinding b;
        b.group=m.sourceChildIds[layer.children.first+g];
        const auto& group=node(b.group,result.layer,SourceNodeKind::ShapeGroup,2,2);
        b.primitive=m.sourceChildIds[group.children.first]; b.fill=m.sourceChildIds[group.children.first+1];
        const auto& primitive=node(b.primitive,b.group,rectangles[g]?SourceNodeKind::Rectangle:SourceNodeKind::Ellipse,
            0,rectangles[g]?3:2);
        const auto& fill=node(b.fill,b.group,SourceNodeKind::Fill,0,2);
        check(primitive.pathDirection==(ccw[g]?SourcePathDirection::CounterClockwise:SourcePathDirection::Clockwise));
        b.groupTransform=prop(group,PropertySemantic::TransformMatrix,PropertyValueType::Matrix3x2);
        b.groupOpacity=prop(group,PropertySemantic::TransformOpacity,PropertyValueType::Scalar);
        b.position=prop(primitive,rectangles[g]?PropertySemantic::RectanglePosition:PropertySemantic::EllipsePosition,PropertyValueType::Vec2);
        b.size=prop(primitive,rectangles[g]?PropertySemantic::RectangleSize:PropertySemantic::EllipseSize,PropertyValueType::Vec2);
        if(rectangles[g]) b.roundness=prop(primitive,PropertySemantic::RectangleRoundness,PropertyValueType::Scalar);
        b.color=prop(fill,PropertySemantic::FillColor,PropertyValueType::Color);
        b.fillOpacity=prop(fill,PropertySemantic::FillOpacity,PropertyValueType::Scalar);
        result.groups.push_back(b);
    }
    check(nodes.size()==23 && edges.size()==22 && properties.size()==48 && propertyEdges.size()==48);
    return result;
}

} // namespace avemotion::test
