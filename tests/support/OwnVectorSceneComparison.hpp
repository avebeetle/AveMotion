#pragma once
#include "OwnVectorSceneOracle.hpp"
namespace avemotion::test::vector_scene {
using namespace model;
std::string role(const MotionAssetModel &m, SourceNodeId id) {
    const auto &n = m.sourceNodes.at(id.index());
    if (n.kind == SourceNodeKind::Composition)
        return "composition";
    if (n.kind == SourceNodeKind::Layer) {
        std::string scope = "root";
        if (const auto *parent = m.sourceNode(n.parent);
            parent && parent->layerKind == SourceLayerKind::Precomposition)
            scope = "precomp:" + std::to_string(parent->authoredLayerId);
        else if (n.composition.valid() && n.composition.index() != 0)
            for (const auto &candidate : m.sourceNodes)
                if (candidate.layerKind == SourceLayerKind::Precomposition &&
                    candidate.referencedComposition == n.composition)
                    scope = "precomp:" + std::to_string(candidate.authoredLayerId);
        return scope + "/layer:" + std::to_string(n.authoredLayerId);
    }
    return role(m, n.parent) + "/" + std::to_string(static_cast<int>(n.kind));
}
using Roles = std::map<std::string, SourceNodeId>;
Roles roles(const MotionAssetModel &m) {
    Roles result;
    for (const auto &n : m.sourceNodes)
        if (n.kind != SourceNodeKind::Composition)
            vectorRequire(result.emplace(role(m, n.id), n.id).second, "unique semantic role");
    return result;
}
double maximum = 0;
std::set<std::string> permittedInactiveMiter;
std::set<std::string> observedInactiveMiter;
// This test-only source walk independently checks whether the author omitted
// ml; the comparator never infers omission from either compiled model's value.
void inspectMiterSource(const std::string &json) {
    using namespace formats::detail;
    const auto read = readOwnJson(json, {65536, 32});
    vectorRequire(bool(read), "miter source read");
    const auto &d = *read.document;
    const auto member = [&](OwnJsonNodeId id, const char *key) {
        for (auto c = d.node(id)->firstChild; c != OwnJsonNoNode; c = d.node(c)->nextSibling)
            if (d.memberName(c) == key)
                return c;
        return OwnJsonNoNode;
    };
    struct Pending {
        OwnJsonNodeId id;
        std::string layer;
        std::string scope = "root";
    };
    std::map<std::string, std::string> assetScopes;
    const auto rootLayers = member(0, "layers");
    for (auto l = d.node(rootLayers)->firstChild; l != OwnJsonNoNode; l = d.node(l)->nextSibling) {
        const auto ref = member(l, "refId");
        if (ref != OwnJsonNoNode)
            assetScopes[std::string(*d.valueBytes(ref))] =
                "precomp:" + std::to_string(static_cast<int>(
                                 std::stof(std::string(*d.valueBytes(member(l, "ind"))))));
    }
    std::vector<Pending> pending{{0, {}}};
    while (!pending.empty()) {
        auto current = pending.back();
        pending.pop_back();
        const auto *node = d.node(current.id);
        if (node->kind == OwnJsonKind::Object) {
            const auto assetId = member(current.id, "id");
            if (assetId != OwnJsonNoNode) {
                const auto scope = assetScopes.find(std::string(*d.valueBytes(assetId)));
                if (scope != assetScopes.end())
                    current.scope = scope->second;
            }
            const auto type = member(current.id, "ty");
            if (type != OwnJsonNoNode && d.node(type)->kind == OwnJsonKind::Number) {
                const auto ind = member(current.id, "ind");
                if (ind != OwnJsonNoNode)
                    current.layer = current.scope + "/layer:" +
                                    std::to_string(static_cast<int>(
                                        std::stof(std::string(*d.valueBytes(ind)))));
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
            pending.push_back({child, current.layer, current.scope});
    }
}

inline std::map<std::string, double> maxima;
inline bool collectNumericFailures = false;
inline std::size_t numericFailures = 0;
inline void near(float expected, float actual, const std::string &field,
                 const std::string &context) {
    const auto error = std::abs(static_cast<double>(expected) - actual);
    maxima[field] = std::max(maxima[field], error);
    if (collectNumericFailures && error > 1e-4) {
        if (numericFailures < 30)
            std::cerr << context << ' ' << field << " expected=" << expected << " actual=" << actual
                      << " error=" << error << '\n';
        ++numericFailures;
        return;
    }
    vectorRequire(std::isfinite(error) && error <= 1e-4,
                  context + " " + field + " expected=" + std::to_string(expected) +
                      " actual=" + std::to_string(actual) + " error=" + std::to_string(error));
}
inline void path(const runtime::EvaluatedPath &ref, const runtime::EvaluatedPath &own,
                 const std::string &field, const std::string &context) {
    vectorRequire(ref.verbs == own.verbs && ref.points.size() == own.points.size(),
                  context + " " + field + " exact topology");
    for (std::size_t i = 0; i < ref.points.size(); ++i) {
        near(ref.points[i].x, own.points[i].x, field + "X",
             context + " point=" + std::to_string(i));
        near(ref.points[i].y, own.points[i].y, field + "Y",
             context + " point=" + std::to_string(i));
    }
    vectorRequire(ref.controlBounds.valid == own.controlBounds.valid, context + " bounds validity");
    if (ref.controlBounds.valid) {
        near(ref.controlBounds.left, own.controlBounds.left, field + "Bounds", context);
        near(ref.controlBounds.top, own.controlBounds.top, field + "Bounds", context);
        near(ref.controlBounds.right, own.controlBounds.right, field + "Bounds", context);
        near(ref.controlBounds.bottom, own.controlBounds.bottom, field + "Bounds", context);
    }
}
inline void paint(const runtime::EvaluatedPaint &ref, const runtime::EvaluatedPaint &own,
                  const std::string &context) {
    vectorRequire(ref.kind == runtime::PaintKind::Solid && own.kind == ref.kind &&
                      ref.solid.r == own.solid.r && ref.solid.g == own.solid.g &&
                      ref.solid.b == own.solid.b && ref.solid.a == own.solid.a,
                  context + " exact solid RGBA");
    vectorRequire(!ref.image.present && !own.image.present && ref.gradient.stops.empty() &&
                      own.gradient.stops.empty(),
                  context + " no hidden paint features");
}
inline void stroke(const runtime::EvaluatedStroke &ref, const runtime::EvaluatedStroke &own,
                   const std::string &key, const std::string &context, bool allowMiter) {
    vectorRequire(ref.enabled == own.enabled && ref.cap == own.cap && ref.join == own.join &&
                      ref.dashArray == own.dashArray,
                  context + " stroke enums/dashes");
    near(ref.width, own.width, "strokeWidth", context);
    if (allowMiter && ref.enabled && permittedInactiveMiter.contains(key) &&
        ref.join != runtime::LineJoin::Miter) {
        vectorRequire(ref.miterLimit == 0 && own.miterLimit == 4,
                      context + " exact omitted inactive miter 0/4");
        observedInactiveMiter.insert(key);
    } else
        near(ref.miterLimit, own.miterLimit, "miter", context);
}
inline std::map<std::string, double> rawLocalMaxima;
inline std::map<std::string, std::string> rawLocalWorstContext;
inline std::size_t rawLocalOverLimit = 0;
inline runtime::RectF pointBounds(const runtime::EvaluatedPath &p) {
    runtime::RectF b;
    for (const auto v : p.points) {
        if (!b.valid)
            b = {true, v.x, v.y, v.x, v.y};
        else {
            b.left = std::min(b.left, v.x);
            b.top = std::min(b.top, v.y);
            b.right = std::max(b.right, v.x);
            b.bottom = std::max(b.bottom, v.y);
        }
    }
    return b;
}
inline void checkBounds(const runtime::EvaluatedPath &p, const std::string &context) {
    const auto b = pointBounds(p);
    vectorRequire(b.valid == p.controlBounds.valid, context + " raw bounds validity");
    if (!b.valid)
        return;
    near(b.left, p.controlBounds.left, "rawBoundsIntegrity", context);
    near(b.top, p.controlBounds.top, "rawBoundsIntegrity", context);
    near(b.right, p.controlBounds.right, "rawBoundsIntegrity", context);
    near(b.bottom, p.controlBounds.bottom, "rawBoundsIntegrity", context);
}
inline void compareLocal(const runtime::EvaluatedPath &ref, const runtime::EvaluatedPath &own,
                         const VMatrix &residual, const std::string &context) {
    checkBounds(ref, context + " reference");
    checkBounds(own, context + " own");
    auto predicted = own;
    for (auto &point : predicted.points) {
        const auto p = residual.map(point.x, point.y);
        point = {p.x(), p.y()};
    }
    predicted.controlBounds = pointBounds(predicted);
    path(ref, predicted, "predictedLocal", context);
}
inline void recordRawLocal(const runtime::EvaluatedPath &a, const runtime::EvaluatedPath &b,
                           const std::string &context) {
    vectorRequire(a.verbs == b.verbs && a.points.size() == b.points.size(),
                  context + " local exact topology");
    const auto record = [&](float x, float y, const std::string &field) {
        const auto error = std::abs(static_cast<double>(x) - y);
        if (error > rawLocalMaxima[field]) {
            rawLocalMaxima[field] = error;
            rawLocalWorstContext[field] = context;
        }
        if (error > 1e-4)
            ++rawLocalOverLimit;
    };
    for (std::size_t i = 0; i < a.points.size(); ++i) {
        record(a.points[i].x, b.points[i].x, "localPathX");
        record(a.points[i].y, b.points[i].y, "localPathY");
    }
    if (a.controlBounds.valid && b.controlBounds.valid) {
        record(a.controlBounds.left, b.controlBounds.left, "localBounds");
        record(a.controlBounds.top, b.controlBounds.top, "localBounds");
        record(a.controlBounds.right, b.controlBounds.right, "localBounds");
        record(a.controlBounds.bottom, b.controlBounds.bottom, "localBounds");
    }
}
inline void compare(const runtime::EvaluatedScene &ref, const model::MotionAssetModel &rm,
                    const runtime::EvaluatedScene &own, const model::MotionAssetModel &om,
                    const std::map<std::uint32_t, VMatrix> &matrices) {
    vectorRequire(ref.drawItems.size() == own.drawItems.size(),
                  "visible draw count ref=" + std::to_string(ref.drawItems.size()) +
                      " own=" + std::to_string(own.drawItems.size()));
    std::set<std::string> seen;
    for (std::size_t i = 0; i < ref.drawItems.size(); ++i) {
        const auto &a = ref.drawItems[i];
        const auto &b = own.drawItems[i];
        const auto key = role(rm, a.sourcePaintNode);
        const auto context =
            "frame=" + std::to_string(own.frameIndex) + " draw=" + std::to_string(i) + " " + key;
        vectorRequire(key == role(om, b.sourcePaintNode) &&
                          role(rm, a.sourcePathNode) == role(om, b.sourcePathNode),
                      context + " ordered source roles");
        vectorRequire(seen.insert(key).second, context + " unique visible role");
        vectorRequire(a.fillRule == b.fillRule, context + " fill rule");
        vectorRequire(a.drawOrder == b.drawOrder, context + " visible draw ordinal");
        vectorRequire(a.sourcePathCount == b.sourcePathCount &&
                          a.sourcePathModifierFree == b.sourcePathModifierFree,
                      context + " path bindings/modifier");
        vectorRequire(a.localGeometryAvailable == b.localGeometryAvailable &&
                          a.localPaintAvailable == b.localPaintAvailable &&
                          a.opacitySeparated == b.opacitySeparated,
                      context + " available local/opacity seams");
        path(a.path, b.path, "finalPath", context);
        const auto *sourcePath = rm.sourceNode(a.sourcePathNode);
        const auto *sourcePaint = rm.sourceNode(a.sourcePaintNode);
        vectorRequire(sourcePath && sourcePaint && sourcePath->parent == sourcePaint->parent &&
                          a.sourcePathCount == 1,
                      context + " same-group single-path applicability");
        const auto &m = matrices.at(a.sourcePaintNode.value);
        vectorRequire(m.isAffine() && m.m_13() == 0 && m.m_23() == 0 && m.m_33() == 1,
                      context + " affine pinned matrix");
        bool invertible = false;
        const auto inverse = m.inverted(&invertible);
        vectorRequire(invertible, context + " invertible pinned matrix");
        recordRawLocal(a.localPath, b.localPath, context);
        compareLocal(a.localPath, b.localPath, m * inverse, context);
        if (collectNumericFailures)
            path(a.localPath, b.localPath, "rawLocalDiagnostic", context);
        near(a.localToViewport.m11, b.localToViewport.m11, "matrix", context);
        near(a.localToViewport.m12, b.localToViewport.m12, "matrix", context);
        near(a.localToViewport.m21, b.localToViewport.m21, "matrix", context);
        near(a.localToViewport.m22, b.localToViewport.m22, "matrix", context);
        near(a.localToViewport.dx, b.localToViewport.dx, "matrix", context);
        near(a.localToViewport.dy, b.localToViewport.dy, "matrix", context);
        near(a.separatedOpacity, b.separatedOpacity, "separatedOpacity", context);
        paint(a.paint, b.paint, context + " final");
        stroke(a.stroke, b.stroke, key, context, true);
        if (a.localPaintAvailable)
            paint(a.localPaint, b.localPaint, context + " local");
        else
            vectorRequire(a.localPaint.kind == b.localPaint.kind,
                          context + " inactive local paint");
        stroke(a.localStroke, b.localStroke, key, context + " local", false);
        const auto &al = ref.layers.at(a.layerIndex);
        const auto &bl = own.layers.at(b.layerIndex);
        vectorRequire(al.visible == bl.visible && al.matte == bl.matte &&
                          al.maskCount == bl.maskCount && al.clipPath.verbs == bl.clipPath.verbs,
                      context + " visible layer features");
        near(al.opacity, bl.opacity, "layerOpacity", context);
    }
}
} // namespace avemotion::test::vector_scene
