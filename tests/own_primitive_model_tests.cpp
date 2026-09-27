#include "OwnPrimitiveTestData.hpp"
#include "OwnNativeEllipseModel.hpp"
#include "OwnJsonReader.hpp"

#include <iostream>
#include <stdexcept>
#include <array>
#include <functional>

namespace {
void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
using namespace avemotion::model;
using namespace avemotion::runtime::detail;

void wholeTables(const OwnNativeEllipseModel& owner) {
    const auto& m = *owner.model;
    const std::array<const char*, 7> names{"Sharp CW", "Rounded CW", "Rounded CCW Clamp",
        "Ellipse CW", "Ellipse CCW", "Animated Rounded CW", "Animated Ellipse CCW"};
    const std::array<bool, 7> rectangles{true,true,true,false,false,true,false};
    const std::array<SourcePathDirection, 7> directions{SourcePathDirection::Clockwise,
        SourcePathDirection::Clockwise, SourcePathDirection::CounterClockwise,
        SourcePathDirection::Clockwise, SourcePathDirection::CounterClockwise,
        SourcePathDirection::Clockwise, SourcePathDirection::CounterClockwise};
    const std::array<std::uint32_t, 7> firstProperties{2,9,16,23,29,35,42};
    const std::array<MotionVec2Value, 7> positions{{{45,42},{125,42},{215,42},{45,115},
        {125,115},{205,105},{75,185}}};
    const std::array<MotionVec2Value, 7> ends{{{45,42},{125,42},{215,42},{45,115},
        {125,115},{225,132},{110,190}}};
    const std::array<MotionVec2Value, 7> sizes{{{54,34},{64,42},{64,42},{58,38},
        {58,38},{44,28},{38,28}}};
    const std::array<MotionVec2Value, 7> sizeEnds{{{54,34},{64,42},{64,42},{58,38},
        {58,38},{76,52},{72,50}}};
    const std::array<float, 7> radii{0,11,80,0,0,0,0};
    const std::array<MotionColorValue, 7> colors{{{1,.2F,.2F,1},{.2F,1,.2F,1},
        {.2F,.4F,1,1},{1,.7F,.1F,1},{.8F,.2F,1,1},{.1F,.9F,.9F,1},{.9F,.1F,.7F,1}}};
    require(owner.input->groups.size() == 7 && owner.values.groups.size() == 7
        && owner.binding.groups.size() == 7, "one canonical seven-group payload");
    require(m.sourceNodes[0].children.first == 0 && m.sourceNodes[0].children.count == 1
        && m.sourceChildIds[0] == makeId<SourceNodeId>(1)
        && m.sourceNodes[1].children.first == 1 && m.sourceNodes[1].children.count == 7,
        "root/layer child ranges");
    for (std::size_t g = 0; g < 7; ++g) {
        const auto group = makeId<SourceNodeId>(2+3*g);
        const auto primitive = makeId<SourceNodeId>(3+3*g);
        const auto fill = makeId<SourceNodeId>(4+3*g);
        const auto p = firstProperties[g];
        const auto& b = owner.binding.groups[g];
        require(b.group == group && b.primitive == primitive && b.fill == fill
            && b.groupTransform == makeId<PropertyId>(p) && b.groupOpacity == makeId<PropertyId>(p+1)
            && b.position == makeId<PropertyId>(p+2) && b.size == makeId<PropertyId>(p+3)
            && b.color == makeId<PropertyId>(p+4+(rectangles[g]?1:0))
            && b.fillOpacity == makeId<PropertyId>(p+5+(rectangles[g]?1:0)), "literal group binding IDs");
        require(b.roundness.has_value() == rectangles[g]
            && (!rectangles[g] || *b.roundness == makeId<PropertyId>(p+4)), "roundness binding");
        require(m.sourceChildIds[1+g] == group && m.sourceChildIds[8+2*g] == primitive
            && m.sourceChildIds[9+2*g] == fill, "ordered source edges");
        const auto& gn = m.sourceNodes[group.index()];
        const auto& pn = m.sourceNodes[primitive.index()];
        const auto& fn = m.sourceNodes[fill.index()];
        require(gn.parent == makeId<SourceNodeId>(1) && pn.parent == group && fn.parent == group
            && gn.kind == SourceNodeKind::ShapeGroup && fn.kind == SourceNodeKind::Fill
            && pn.kind == (rectangles[g]?SourceNodeKind::Rectangle:SourceNodeKind::Ellipse)
            && pn.pathDirection == directions[g] && gn.debugName == names[g]
            && pn.debugName == names[g] && fn.debugName == "Fill", "literal source roles/names/direction");
        require(gn.children.first == 8+2*g && gn.children.count == 2 && pn.children.count == 0
            && fn.children.count == 0 && gn.properties.first == p && gn.properties.count == 2
            && pn.properties.first == p+2 && pn.properties.count == (rectangles[g]?3U:2U)
            && fn.properties.first == b.color.index() && fn.properties.count == 2,
            "group child/property ranges");
        const auto& v = owner.values.groups[g];
        const auto checkVector = [&](const OwnPrimitiveVectorValues& actual, MotionVec2Value start,
                                     MotionVec2Value end, PropertyId id, PropertySemantic semantic) {
            require(actual.start == start && actual.end == end && actual.animated == (g>=5)
                && actual.firstFrame == 0 && actual.lastFrame == (g>=5?60U:0U)
                && actual.outgoing == (g>=5?MotionVec2Value{.333F,0}:MotionVec2Value{})
                && actual.incoming == (g>=5?MotionVec2Value{.667F,1}:MotionVec2Value{}),
                "literal numeric vector endpoints/timing/controls");
            const auto& prop = m.properties[id.index()];
            require(prop.owner == primitive && prop.semantic == semantic
                && prop.valueType == PropertyValueType::Vec2, "primitive property role/type");
            if (g < 5) require(m.vec2Values[prop.staticValue.index] == start, "static model vector");
            else {
                const auto& track = m.tracks[prop.track.index()];
                const auto& segment = m.segments[track.segments.first];
                require(track.property == id && track.firstFrame == 0 && track.endFrame == 60
                    && track.segments.count == 1 && segment.track == track.id
                    && segment.firstFrame == 0 && segment.endFrame == 60
                    && segment.interpolation == SegmentInterpolation::CubicBezier
                    && segment.temporalControl1 == MotionVec2Value{.333F,0}
                    && segment.temporalControl2 == MotionVec2Value{.667F,1}
                    && m.vec2Values[segment.startValue.index] == start
                    && m.vec2Values[segment.endValue.index] == end, "literal animated model vector");
            }
        };
        checkVector(v.position, positions[g], ends[g], b.position,
            rectangles[g]?PropertySemantic::RectanglePosition:PropertySemantic::EllipsePosition);
        checkVector(v.size, sizes[g], sizeEnds[g], b.size,
            rectangles[g]?PropertySemantic::RectangleSize:PropertySemantic::EllipseSize);
        require(v.color == colors[g] && m.colorValues[m.properties[b.color.index()].staticValue.index]
            == colors[g], "literal per-group numeric and model color");
        if (rectangles[g]) {
            require(v.roundness && v.roundness->start == radii[g]
                && v.roundness->end == (g==5?20:radii[g]) && v.roundness->animated == (g==5)
                && v.roundness->firstFrame == 0 && v.roundness->lastFrame == (g==5?60U:0U)
                && v.roundness->outgoing == (g==5?MotionVec2Value{.333F,0}:MotionVec2Value{})
                && v.roundness->incoming == (g==5?MotionVec2Value{.667F,1}:MotionVec2Value{}),
                "literal radius endpoints/timing/controls");
            const auto& prop = m.properties[b.roundness->index()];
            require(prop.owner == primitive && prop.semantic == PropertySemantic::RectangleRoundness
                && prop.valueType == PropertyValueType::Scalar, "radius model role/type");
            if (g==5) {
                const auto& s = m.segments[m.tracks[prop.track.index()].segments.first];
                require(m.scalarValues[s.startValue.index] == 0 && m.scalarValues[s.endValue.index] == 20,
                    "literal scalar track endpoints");
            } else require(m.scalarValues[prop.staticValue.index] == radii[g], "literal static model radius");
        } else require(!v.roundness, "ellipse has no radius");
    }
    require(static_cast<bool>(bindOwnPrimitiveModel(*owner.input, m)), "whole source binding");
    const std::array<std::function<void(MotionAssetModel&)>, 10> corruptions{
        [](auto& a){a.sourceChildIds.back()=a.sourceChildIds.front();},
        [](auto& a){a.sourceChildIds.push_back(makeId<SourceNodeId>(22));},
        [](auto& a){a.sourceNodes.back().parent=makeId<SourceNodeId>(17);},
        [](auto& a){a.sourceNodes[20].properties=a.sourceNodes[17].properties;},
        [](auto& a){a.properties.back().owner=makeId<SourceNodeId>(19);},
        [](auto& a){a.sourcePropertyIds.back()=a.sourcePropertyIds.front();},
        [](auto& a){a.properties.back().staticValue=a.properties[1].staticValue;},
        [](auto& a){a.vec2Values.back().x+=1;},
        [](auto& a){a.segments.back().endFrame=59;},
        [](auto& a){a.tracks.back().property=makeId<PropertyId>(1);}
    };
    for (const auto& corrupt : corruptions) {
        auto copy=m; corrupt(copy);
        const auto bad=bindOwnPrimitiveModel(*owner.input,copy);
        require(!bad && !bad.binding, "later mutation cannot publish a partial binding");
    }
}
}

int main() {
    try {
        const auto document = avemotion::formats::detail::readOwnJson(avemotion::test::primitiveFixture());
        require(static_cast<bool>(document), "fixture parses");
        const auto built = avemotion::runtime::detail::buildOwnNativeEllipseModel(*document.document);
        require(static_cast<bool>(built), "whole fixture factory must return Prepared (seven primitives)");
        const auto& model = *built.prepared->model;
        require(model.sourceNodes.size() == 23 && model.sourceChildIds.size() == 22,
                "all seven source triples and edges survive factory");
        require(model.properties.size() == 48 && model.statistics.staticPropertyCount == 43
                && model.statistics.animatedPropertyCount == 5 && model.tracks.size() == 5
                && model.segments.size() == 5, "all five animations survive factory");
        wholeTables(*built.prepared);
        const auto invalid = avemotion::formats::detail::readOwnJson(avemotion::test::replacePrimitiveOnce(
            avemotion::test::primitiveFixture(), "\"nm\": \"Animated Ellipse CCW\",", "\"unsupported\":true,"));
        require(static_cast<bool>(invalid), "later invalid group parses");
        const auto rejected = buildOwnNativeEllipseModel(*invalid.document);
        require(!rejected && !rejected.prepared && rejected.code == OwnNativeEllipseModelCode::AdmissionRejected,
            "later group admission is atomic");
        std::cout << "own primitive model tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
