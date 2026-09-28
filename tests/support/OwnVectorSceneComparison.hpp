#pragma once
#include "OwnVectorSceneOracle.hpp"
namespace avemotion::test::vector_scene {
using namespace model;
class OwnRoles {
  public:
    explicit OwnRoles(const render::detail::OwnNativeEllipsePreparedAsset &prepared)
        : prepared_(prepared), authored_(*prepared.vectorAuthored) {
        const auto read = formats::detail::readOwnJson(authored_.exactJson, {65536, 32});
        vectorRequire(bool(read), "own provenance JSON");
        document_ = read.document;
        std::function<void(formats::detail::OwnJsonNodeId, std::string)> walk =
            [&](auto id, std::string pointer) {
                pointers_[pointer] = id;
                const auto *node = document_->node(id);
                unsigned index = 0;
                for (auto child = node->firstChild; child != formats::detail::OwnJsonNoNode;
                     child = document_->node(child)->nextSibling, ++index)
                    walk(child, pointer + "/" +
                                    (node->kind == formats::detail::OwnJsonKind::Array
                                         ? std::to_string(index)
                                         : std::string(*document_->memberName(child))));
            };
        walk(0, "");
    }
    std::string value(const std::string &pointer) const {
        return std::string(*document_->valueBytes(pointers_.at(pointer)));
    }
    std::string instance(SourceNodeId id) const {
        const auto &node = authored_.model->sourceNodes.at(id.index());
        if (node.kind == SourceNodeKind::Composition)
            return "root";
        const auto &source = authored_.sources.at(id.index());
        const auto &parent = authored_.model->sourceNodes.at(source.containingInstance.index());
        auto prefix = instance(source.containingInstance);
        if (parent.kind == SourceNodeKind::Layer)
            prefix += "->" + value(authored_.sources.at(parent.id.index()).jsonPointer + "/refId");
        return prefix + "/layer:" + std::to_string(node.authoredLayerId) + "[" +
               source.jsonPointer.substr(source.jsonPointer.rfind('/') + 1) + "]";
    }
    SourceNodeId layer(SourceNodeId id) const {
        while (authored_.model->sourceNodes.at(id.index()).kind != SourceNodeKind::Layer)
            id = authored_.model->sourceNodes.at(id.index()).parent;
        return id;
    }
    std::string definition(SourceNodeId id) const {
        const auto owner = layer(id);
        const auto &base = authored_.sources.at(owner.index()).jsonPointer;
        std::string composition = "root";
        if (base.starts_with("/assets/"))
            composition = value(base.substr(0, base.find("/layers/")) + "/id");
        return composition + "/layer:" +
               std::to_string(authored_.model->sourceNodes.at(owner.index()).authoredLayerId) +
               authored_.sources.at(id.index()).jsonPointer.substr(base.size());
    }
    std::string draw(const runtime::EvaluatedScene &scene,
                     const runtime::EvaluatedDrawItem &draw) const {
        const auto owner = layer(draw.sourcePathNode);
        vectorRequire(owner == layer(draw.sourcePaintNode), "own path and paint share instance");
        const auto found =
            std::find_if(prepared_.program->layers.begin(), prepared_.program->layers.end(),
                         [&](const auto &binding) { return binding.source == owner; });
        vectorRequire(found != prepared_.program->layers.end() &&
                          found->layer == scene.layers.at(draw.layerIndex).modelLayer,
                      "own draw layer agrees with provenance");
        for (auto id = owner;
             authored_.model->sourceNodes.at(id.index()).kind != SourceNodeKind::Composition;) {
            const auto &source = authored_.sources.at(id.index());
            vectorRequire(authored_.model->sourceNodes.at(id.index()).parent ==
                              source.containingInstance,
                          "own structural source edge");
            id = source.containingInstance;
        }
        return instance(owner) + "|" + definition(draw.sourcePathNode) + "|" +
               definition(draw.sourcePaintNode);
    }
    bool omittedMiter(SourceNodeId id) const {
        const auto &p = authored_.sources.at(id.index()).jsonPointer;
        return !pointers_.contains(p + "/ml") && pointers_.contains(p + "/lj") &&
               value(p + "/lj") != "1";
    }
    int clock(SourceNodeId id) const { return authored_.sources.at(id.index()).frameOffset; }

  private:
    const render::detail::OwnNativeEllipsePreparedAsset &prepared_;
    const runtime::detail::OwnVectorModel &authored_;
    std::shared_ptr<const formats::detail::OwnJsonDocument> document_;
    std::map<std::string, formats::detail::OwnJsonNodeId> pointers_;
};
inline std::set<std::string> permittedInactiveMiter;
inline std::set<std::string> observedInactiveMiter;
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
inline void compare(const runtime::EvaluatedScene &ref, const OwnVectorSceneOracle &oracle,
                    const runtime::EvaluatedScene &own, const OwnRoles &ownRoles) {
    oracle.validateInstances(ref);
    oracle.assertRedundantClips(ref);
    if (ref.drawItems.size() != own.drawItems.size()) {
        std::set<std::string> expected;
        for (const auto &draw : ref.drawItems)
            expected.insert(oracle.drawRole(ref, draw));
        for (const auto &draw : own.drawItems) {
            const auto key = ownRoles.draw(own, draw);
            if (!expected.contains(key))
                std::cerr << "unexpected own draw frame=" << own.frameIndex << ' ' << key
                          << " alpha=" << draw.separatedOpacity << '\n';
        }
    }
    vectorRequire(ref.drawItems.size() == own.drawItems.size(),
                  "frame=" + std::to_string(own.frameIndex) +
                      " visible draw count ref=" + std::to_string(ref.drawItems.size()) +
                      " own=" + std::to_string(own.drawItems.size()));
    std::set<std::string> seen;
    for (std::size_t i = 0; i < ref.drawItems.size(); ++i) {
        const auto &a = ref.drawItems[i];
        const auto &b = own.drawItems[i];
        const auto key = oracle.drawRole(ref, a);
        const auto context =
            "frame=" + std::to_string(own.frameIndex) + " draw=" + std::to_string(i) + " " + key;
        vectorRequire(key == ownRoles.draw(own, b), context + " ordered source roles");
        const auto expectedOffset =
            oracle.instances().at(ref.layers.at(a.layerIndex).modelLayer.value).offset;
        vectorRequire(ownRoles.clock(b.sourcePathNode) == expectedOffset &&
                          ownRoles.clock(b.sourcePaintNode) == expectedOffset,
                      context + " independently derived containing-composition clock");
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
        vectorRequire(a.sourcePathCount == 1, context + " single-path applicability");
        const auto &m = oracle.matrices().at(a.modelNode.value);
        const auto group = oracle.sourceWorld(ref, a, a.sourcePathNode);
        const auto paintMatrix = oracle.sourceWorld(ref, a, a.sourcePaintNode);
        near(paintMatrix.m_11(), m.m_11(), "referencePaintMatrix", context);
        near(paintMatrix.m_12(), m.m_12(), "referencePaintMatrix", context);
        near(paintMatrix.m_21(), m.m_21(), "referencePaintMatrix", context);
        near(paintMatrix.m_22(), m.m_22(), "referencePaintMatrix", context);
        near(paintMatrix.m_tx(), m.m_tx(), "referencePaintMatrix", context);
        near(paintMatrix.m_ty(), m.m_ty(), "referencePaintMatrix", context);
        vectorRequire(m.isAffine() && m.m_13() == 0 && m.m_23() == 0 && m.m_33() == 1,
                      context + " affine pinned matrix");
        bool invertible = false;
        const auto inverse = m.inverted(&invertible);
        vectorRequire(invertible, context + " invertible pinned matrix");
        recordRawLocal(a.localPath, b.localPath, context);
        compareLocal(a.localPath, b.localPath, group * inverse, context);
        if (collectNumericFailures)
            path(a.localPath, b.localPath, "rawLocalDiagnostic", context);
        near(group.m_11(), b.localToViewport.m11, "matrix", context);
        near(group.m_12(), b.localToViewport.m12, "matrix", context);
        near(group.m_21(), b.localToViewport.m21, "matrix", context);
        near(group.m_22(), b.localToViewport.m22, "matrix", context);
        near(group.m_tx(), b.localToViewport.dx, "matrix", context);
        near(group.m_ty(), b.localToViewport.dy, "matrix", context);
        near(a.separatedOpacity, b.separatedOpacity, "separatedOpacity", context);
        paint(a.paint, b.paint, context + " final");
        if (ownRoles.omittedMiter(b.sourcePaintNode))
            permittedInactiveMiter.insert(key);
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
