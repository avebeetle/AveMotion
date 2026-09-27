#include "OwnVectorTestData.hpp"
#include "TelegramParsedModelBuilder.hpp"
#include "avemotion/evaluation/PropertyEvaluator.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <set>
using namespace avemotion;
using namespace model;
using test::vectorRequire;
namespace {
std::string role(const MotionAssetModel& m, SourceNodeId id) {
    const auto& n = m.sourceNodes.at(id.index());
    if (n.kind == SourceNodeKind::Composition)
        return "composition";
    if (n.kind == SourceNodeKind::Layer)
        return n.layerKind == SourceLayerKind::Precomposition
                   ? "precomp"
                   : "layer:" + std::to_string(n.authoredLayerId);
    return role(m, n.parent) + "/" + std::to_string(static_cast<int>(n.kind));
}
using Roles = std::map<std::string, SourceNodeId>;
Roles roles(const MotionAssetModel& m) {
    Roles result;
    for (const auto& n : m.sourceNodes)
        if (n.kind != SourceNodeKind::Composition)
            vectorRequire(result.emplace(role(m, n.id), n.id).second, "unique semantic role");
    return result;
}
double maximum = 0;
std::set<std::string> permittedInactiveMiter;
std::set<std::string> observedInactiveMiter;
// This test-only source walk independently checks whether the author omitted ml;
// the comparator never infers omission from either compiled model's value.
void inspectMiterSource(const std::string& json) {
    using namespace formats::detail;
    const auto read = readOwnJson(json, {65536, 32});
    vectorRequire(bool(read), "miter source read");
    const auto& d = *read.document;
    const auto member = [&](OwnJsonNodeId id, const char* key) {
        for (auto c = d.node(id)->firstChild; c != OwnJsonNoNode; c = d.node(c)->nextSibling)
            if (d.memberName(c) == key)
                return c;
        return OwnJsonNoNode;
    };
    struct Pending {
        OwnJsonNodeId id;
        std::string layer;
    };
    std::vector<Pending> pending{{0, {}}};
    while (!pending.empty()) {
        auto current = pending.back();
        pending.pop_back();
        const auto* node = d.node(current.id);
        if (node->kind == OwnJsonKind::Object) {
            const auto type = member(current.id, "ty");
            if (type != OwnJsonNoNode && d.node(type)->kind == OwnJsonKind::Number) {
                const auto ind = member(current.id, "ind");
                if (ind != OwnJsonNoNode)
                    current.layer = "layer:" + std::string(*d.valueBytes(ind));
            }
            if (type != OwnJsonNoNode && d.valueBytes(type) == "st" &&
                member(current.id, "ml") == OwnJsonNoNode) {
                const auto join = member(current.id, "lj");
                if (join != OwnJsonNoNode &&
                    (d.valueBytes(join) == "2" || d.valueBytes(join) == "3"))
                    permittedInactiveMiter.insert(
                        current.layer + "/" +
                        std::to_string(static_cast<int>(SourceNodeKind::ShapeGroup)) + "/" +
                        std::to_string(static_cast<int>(SourceNodeKind::Stroke)));
            }
        }
        for (auto child = node->firstChild; child != OwnJsonNoNode;
             child = d.node(child)->nextSibling)
            pending.push_back({child, current.layer});
    }
}
void close(float expected, float got, const std::string& context) {
    const auto difference = std::abs(static_cast<double>(expected) - got);
    maximum = std::max(maximum, difference);
    vectorRequire(difference <= 1e-4, context + " expected=" + std::to_string(expected) +
                                          " got=" + std::to_string(got) +
                                          " error=" + std::to_string(difference));
}
void matrix(MotionMatrix3x2Value a, MotionMatrix3x2Value b, const std::string& c) {
    close(a.m11, b.m11, c + " m11");
    close(a.m12, b.m12, c + " m12");
    close(a.m21, b.m21, c + " m21");
    close(a.m22, b.m22, c + " m22");
    close(a.dx, b.dx, c + " dx");
    close(a.dy, b.dy, c + " dy");
}
struct Shape {
    bool closed;
    std::span<const MotionVec2Value> points;
};
Shape shape(const MotionAssetModel& m, const evaluation::MotionPropertyValue& value,
            std::span<const evaluation::EvaluatedShape> shapes,
            std::span<const MotionVec2Value> points) {
    if (value.storage == evaluation::PropertyStorageKind::AssetReference) {
        const auto& s = m.shapeValues.at(value.assetReference.index);
        return {s.closed, std::span(m.shapePoints).subspan(s.points.first, s.points.count)};
    }
    const auto& s = shapes[value.shapeSlot];
    vectorRequire(!s.topologyTruncated, "no truncated morph");
    return {s.closed, points.subspan(s.firstPoint, s.pointCount)};
}
void compare(const model::detail::TelegramPropertyOracleResult& ref,
             const runtime::detail::OwnVectorModel& own,
             const evaluation::PropertyEvaluationView& view, const std::string& context) {
    const auto rr = roles(*ref.model), ownRoles = roles(*own.model);
    vectorRequire(rr.size() == ownRoles.size(), "all source roles covered");
    std::size_t animated = 0;
    for (const auto& [key, rid] : rr) {
        const auto found = ownRoles.find(key);
        vectorRequire(found != ownRoles.end(), "missing own role " + key);
        const auto oid = found->second;
        const auto& rn = ref.model->sourceNodes[rid.index()];
        const auto& on = own.model->sourceNodes[oid.index()];
        vectorRequire(rn.kind == on.kind && rn.layerKind == on.layerKind &&
                          rn.fillRule == on.fillRule && rn.strokeCap == on.strokeCap &&
                          rn.strokeJoin == on.strokeJoin && rn.trimMode == on.trimMode,
                      "source enums " + key);
        if (rn.kind == SourceNodeKind::Stroke) {
            if (permittedInactiveMiter.contains(key) && rn.strokeJoin != SourceStrokeJoin::Miter) {
                vectorRequire(rn.miterLimit == 0 && on.miterLimit == 4,
                              context + key + " exact permitted omitted/inactive miter defaults");
                observedInactiveMiter.insert(key);
            } else
                close(rn.miterLimit, on.miterLimit, context + key + " active/authored miter");
        }
        const auto& rt = ref.nodeTransforms[rid.index()];
        const auto& ot = view.nodeTransforms[oid.index()];
        matrix(rt.localMatrix, ot.localMatrix, context + key + " local");
        matrix(rt.worldMatrix, ot.worldMatrix, context + key + " world");
        close(rt.localOpacity, ot.localOpacity, context + key + " local opacity");
        close(rt.worldOpacity, ot.worldOpacity, context + key + " world opacity");
        for (const auto& rp : ref.model->properties)
            if (rp.owner == rid && rp.semantic != PropertySemantic::TransformMatrix) {
                const auto op = std::find_if(
                    own.model->properties.begin(), own.model->properties.end(),
                    [&](const auto& p) { return p.owner == oid && p.semantic == rp.semantic; });
                vectorRequire(op != own.model->properties.end(), "missing property role " + key);
                vectorRequire(rp.valueType == op->valueType, "property type " + key);
                if (rp.flags & PropertyFlagAnimated) {
                    ++animated;
                    vectorRequire(op->flags & PropertyFlagAnimated, "animated role retained");
                }
                const auto& a = ref.properties[rp.id.index()];
                const auto& b = view.properties[op->id.index()].value;
                const auto c =
                    context + key + " semantic=" + std::to_string(static_cast<int>(rp.semantic));
                switch (rp.valueType) {
                case PropertyValueType::Scalar:
                    close(a.scalar, b.scalar, c);
                    break;
                case PropertyValueType::Vec2:
                    close(a.vec2.x, b.vec2.x, c + " x");
                    close(a.vec2.y, b.vec2.y, c + " y");
                    break;
                case PropertyValueType::Color:
                    vectorRequire(a.color == b.color, c + " exact RGBA");
                    break;
                case PropertyValueType::Shape: {
                    const auto sa = shape(*ref.model, a, ref.shapes, ref.shapePoints),
                               sb = shape(*own.model, b, view.shapes, view.shapePoints);
                    vectorRequire(sa.closed == sb.closed && sa.points.size() == sb.points.size(),
                                  c + " exact topology/closure");
                    for (std::size_t i = 0; i < sa.points.size(); ++i) {
                        close(sa.points[i].x, sb.points[i].x, c + " pointX");
                        close(sa.points[i].y, sb.points[i].y, c + " pointY");
                    }
                    break;
                }
                default:
                    vectorRequire(false, "unexpected property type");
                }
            }
    }
    vectorRequire(animated == own.model->tracks.size(),
                  "every own animated property has oracle role");
}
} // namespace
void run(const std::string& json) {
    maximum = 0;
    permittedInactiveMiter.clear();
    observedInactiveMiter.clear();
    inspectMiterSource(json);
    const auto own = test::vectorCompile(json);
    vectorRequire(bool(own), own.path + " " + own.message);
    const auto& m = *own.prepared->model;
    const model::detail::AssetModelDescriptor desc{
        {99, 1},     m.sourceAssetHash, m.logicalWidth,   m.logicalHeight,
        m.frameRate, m.totalFrames,     "own-vector-test"};
    evaluation::PropertyEvaluator evaluator(own.prepared->model);
    evaluation::PropertyEvaluationWorkspace ws;
    evaluator.prepare(ws);
    for (int frame = 0; frame < 180; ++frame) {
        const auto oracle = model::detail::evaluateTelegramParsedProperties(
            json, "own-vector-property-" + std::to_string(frame), desc, frame);
        vectorRequire(bool(oracle), "ordinary Telegram property oracle: " + oracle.error);
        const auto view = evaluator.evaluate(frame, ws);
        vectorRequire(bool(view), "own frame evaluates");
        compare(oracle, *own.prepared, view, "frame=" + std::to_string(frame) + " ");
        if (frame == 0) {
            auto changed = oracle;
            auto changedModel = std::make_shared<MotionAssetModel>(*oracle.model);
            auto stroke = std::find_if(changedModel->sourceNodes.begin(),
                                       changedModel->sourceNodes.end(), [](const auto& n) {
                                           return n.kind == SourceNodeKind::Stroke &&
                                                  n.strokeJoin == SourceStrokeJoin::Miter;
                                       });
            if (stroke != changedModel->sourceNodes.end()) {
                stroke->miterLimit += 1;
                changed.model = changedModel;
                const auto savedMaximum = maximum;
                bool rejected = false;
                try {
                    compare(changed, *own.prepared, view, "active miter mutation ");
                } catch (const std::runtime_error& e) {
                    rejected =
                        std::string(e.what()).find("active/authored miter") != std::string::npos;
                }
                maximum = savedMaximum;
                vectorRequire(rejected, "active miter mutation caught");
            }
            // A changed shape point and a changed parent matrix must fail the
            // same full semantic comparison used on every unmodified frame.
            for (bool shapeMutation : {true, false}) {
                changed = oracle;
                changedModel = std::make_shared<MotionAssetModel>(*oracle.model);
                changed.model = changedModel;
                if (shapeMutation) {
                    vectorRequire(!changedModel->shapePoints.empty(), "shape mutation has source");
                    changedModel->shapePoints[0].x += 1;
                    if (!changed.shapePoints.empty())
                        changed.shapePoints[0].x += 1;
                } else {
                    const auto layer =
                        std::find_if(changedModel->sourceNodes.begin(),
                                     changedModel->sourceNodes.end(), [](const auto& n) {
                                         return n.kind == SourceNodeKind::Layer &&
                                                n.layerKind != SourceLayerKind::Precomposition;
                                     });
                    changed.nodeTransforms[layer->id.index()].worldMatrix.dx += 1;
                }
                const auto savedMaximum = maximum;
                bool rejected = false;
                try {
                    compare(changed, *own.prepared, view, "semantic mutation ");
                } catch (const std::runtime_error&) {
                    rejected = true;
                }
                maximum = savedMaximum;
                vectorRequire(rejected, "semantic comparator mutation caught");
            }
        }
    }
    for (int frame : {180, 200, 15, 14, 179, 0}) {
        const auto oracle = model::detail::evaluateTelegramParsedProperties(
            json, "own-vector-boundary-" + std::to_string(frame), desc, frame);
        vectorRequire(bool(oracle), "boundary oracle");
        compare(oracle, *own.prepared, evaluator.evaluate(frame, ws),
                "boundary=" + std::to_string(frame) + " ");
    }
    std::cout << "PASS ordinary Telegram property oracle frames=180 boundaries=6 "
                 "maximum_absolute_error="
              << maximum << " own_properties=" << m.properties.size()
              << " animated=" << m.tracks.size() << " segments=" << m.segments.size()
              << " permitted_inactive_miter_roles=" << observedInactiveMiter.size() << '\n';
}
int main(int argc, char** argv) {
    try {
        run(test::vectorInput(argc, argv));
        if (argc == 1)
            run(test::vectorRead(std::filesystem::path(AVEMOTION_FIXTURE_DIR) /
                                 "own_vector/animated.json"));
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
