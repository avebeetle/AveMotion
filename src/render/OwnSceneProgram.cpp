#include "OwnSceneProgram.hpp"
#include "NativeEllipseEvaluationHelpers.hpp"
#include "NativeEllipsePathMaterializer.hpp"
#include "OwnNativeEllipseModel.hpp"
#include "OwnVectorModel.hpp"
#include "SourcePathMaterializer.hpp"
#include "TrimPathGenerator.hpp"
#include "avemotion/core/Hash.hpp"
#include <functional>
#include <utility>

namespace avemotion::render::detail {
namespace {
model::PropertyId property(const model::MotionAssetModel &asset, model::SourceNodeId owner,
                           model::PropertySemantic semantic) {
    for (const auto &p : asset.properties)
        if (p.owner == owner && p.semantic == semantic)
            return p.id;
    return {};
}
bool animated(const model::MotionAssetModel &asset, model::PropertyId id) {
    const auto *p = asset.property(id);
    return p && (p->flags & model::PropertyFlagAnimated) != 0;
}
} // namespace
OwnSceneProgram::OwnSceneProgram(std::vector<OwnSceneLayer> l, std::vector<OwnSceneDraw> d)
    : layers(std::move(l)), draws(std::move(d)) {}

std::shared_ptr<const OwnSceneProgram>
lowerOwnPrimitiveProgram(const runtime::detail::OwnNativeEllipseModel &owner,
                         const model::MotionAssetModel &asset) {
    std::vector<OwnSceneLayer> layers{
        {{}, model::makeId<model::LayerId>(0), 0, static_cast<double>(asset.totalFrames), {}},
        {owner.binding.layer,
         model::makeId<model::LayerId>(1),
         static_cast<double>(owner.input->layerInFrame),
         static_cast<double>(owner.input->layerOutFrame),
         {0, static_cast<std::uint32_t>(owner.binding.groups.size())}}};
    std::vector<OwnSceneDraw> draws;
    for (auto it = owner.binding.groups.rbegin(); it != owner.binding.groups.rend(); ++it) {
        OwnSceneDraw draw;
        draw.group = it->group;
        draw.paintScope = it->group;
        draw.path = it->primitive;
        draw.paint = it->fill;
        draw.node = model::makeId<model::NodeId>(draws.size());
        draw.position = it->position;
        draw.size = it->size;
        if (it->roundness)
            draw.roundness = *it->roundness;
        draw.geometryClass = asset.geometries[draws.size()].resourceClass;
        draws.push_back(draw);
    }
    return std::make_shared<const OwnSceneProgram>(std::move(layers), std::move(draws));
}

std::shared_ptr<const OwnSceneProgram>
lowerOwnVectorProgram(const runtime::detail::OwnVectorModel &owner,
                      model::MotionAssetModel &asset) {
    std::vector<OwnSceneLayer> layers;
    std::vector<OwnSceneDraw> draws;
    asset.layers.clear();
    asset.nodes.clear();
    asset.drawOrder.clear();
    asset.childLayerIds.clear();
    asset.layerNodeIds.clear();
    const auto root = asset.compositions.front().rootNode;
    std::function<void(model::SourceNodeId, model::LayerId, double, double,
                       std::vector<model::SourceNodeId>)> append;
    append = [&](model::SourceNodeId source, model::LayerId parent, double first, double last,
                 std::vector<model::SourceNodeId> enclosingClips) {
        const auto id = model::makeId<model::LayerId>(layers.size());
        const auto *node = asset.sourceNode(source);
        const auto name = parent.valid() ? node->debugName : "__";
        core::Fnv1a64 hash;
        hash.appendString(name);
        asset.layers.push_back({true,
                                id,
                                parent,
                                {},
                                {},
                                {},
                                hash.value(),
                                name,
                                model::StaticDependencyNone,
                                runtime::MatteMode::None});
        OwnSceneLayer layer{source, id, first, last, {static_cast<std::uint32_t>(draws.size()), 0}};
        layer.enclosingClips = enclosingClips;
        for (const auto &b : owner.draws) {
            if (b.layer != source)
                continue;
            OwnSceneDraw d;
            d.group = b.group;
            d.paintScope = b.paintScope;
            d.path = b.path;
            d.paint = b.paint;
            d.trim = b.trim;
            d.node = model::makeId<model::NodeId>(draws.size());
            d.shape = property(asset, d.path, model::PropertySemantic::ShapePath);
            const bool stroke = asset.sourceNode(d.paint)->kind == model::SourceNodeKind::Stroke;
            d.paintColor = property(asset, d.paint,
                                    stroke ? model::PropertySemantic::StrokeColor
                                           : model::PropertySemantic::FillColor);
            d.paintOpacity = property(asset, d.paint,
                                      stroke ? model::PropertySemantic::StrokeOpacity
                                             : model::PropertySemantic::FillOpacity);
            if (stroke)
                d.strokeWidth = property(asset, d.paint, model::PropertySemantic::StrokeWidth);
            d.fillRule = asset.sourceNode(d.paint)->fillRule == model::SourceFillRule::EvenOdd
                             ? runtime::FillRule::EvenOdd
                             : runtime::FillRule::Winding;
            if (d.trim) {
                d.trimStart = property(asset, *d.trim, model::PropertySemantic::TrimStart);
                d.trimEnd = property(asset, *d.trim, model::PropertySemantic::TrimEnd);
                d.trimOffset = property(asset, *d.trim, model::PropertySemantic::TrimOffset);
            }
            d.geometryClass = animated(asset, d.shape) || animated(asset, d.trimStart) ||
                                      animated(asset, d.trimEnd) || animated(asset, d.trimOffset)
                                  ? model::ResourceClass::InstanceEvaluated
                                  : model::ResourceClass::AssetStatic;
            const auto slot = static_cast<std::uint32_t>(draws.size());
            asset.nodes.push_back(
                {true, d.node, model::makeId<model::DrawItemId>(slot), id,
                 model::makeId<model::GeometryId>(slot), model::makeId<model::PaintId>(slot), slot,
                 model::StaticDependencyTransform | model::StaticDependencyVisibility |
                     (d.geometryClass == model::ResourceClass::InstanceEvaluated
                          ? model::StaticDependencyGeometry
                          : 0U)});
            asset.layerNodeIds.push_back(d.node);
            asset.drawOrder.push_back(d.node);
            draws.push_back(d);
            ++layer.draws.count;
        }
        asset.layers[id.index()].nodes = layer.draws;
        layers.push_back(layer);
        if (node->layerKind == model::SourceLayerKind::Precomposition)
            enclosingClips.push_back(source);
        // Source allocation is independent of structural stacking. Traverse each
        // composition's authored layers backwards, expanding containers in place.
        for (auto it = owner.layers.rbegin(); it != owner.layers.rend(); ++it)
            if (it->structuralParent == source)
                append(it->layer, id, it->inFrame, it->outFrame, enclosingClips);
        auto &record = asset.layers[id.index()];
        record.children.first = static_cast<std::uint32_t>(asset.childLayerIds.size());
        for (const auto &child : asset.layers)
            if (child.parent == id)
                asset.childLayerIds.push_back(child.id);
        record.children.count =
            static_cast<std::uint32_t>(asset.childLayerIds.size()) - record.children.first;
    };
    append(root, {}, 0, static_cast<double>(asset.totalFrames), {});
    return std::make_shared<const OwnSceneProgram>(std::move(layers), std::move(draws));
}

bool materializeOwnScenePath(const model::MotionAssetModel &asset, const OwnSceneDraw &draw,
                             const evaluation::PropertyEvaluationView &view,
                             const runtime::AffineTransform &transform,
                             runtime::EvaluatedPath &local, runtime::EvaluatedPath &final) {
    const auto *source = asset.sourceNode(draw.path);
    if (!source)
        return false;
    if (source->kind != model::SourceNodeKind::Shape) {
        model::MotionVec2Value position, size;
        float roundness = 0;
        if (!resolveNativeEllipseVec2(asset, view, draw.position, position) ||
            !resolveNativeEllipseVec2(asset, view, draw.size, size) ||
            (draw.roundness.valid() &&
             !resolveNativeEllipseScalar(asset, view, draw.roundness, roundness)))
            return false;
        const auto path =
            source->kind == model::SourceNodeKind::Rectangle
                ? generateRectanglePath(position, size, roundness, source->pathDirection)
                : generateEllipsePath(position, size, source->pathDirection);
        return materializeNativeEllipsePath(path, nullptr, local) &&
               materializeNativeEllipsePath(path, &transform, final);
    }
    const auto *p = asset.property(draw.shape);
    ShapeSample shape;
    std::vector<runtime::PathVerb> verbs;
    std::vector<model::MotionVec2Value> points;
    if (!p || !resolveShape(asset, *p, view, shape) || !shapePathStream(shape, verbs, points))
        return false;
    if (draw.trim) {
        float start, end, offset;
        if (!resolveNativeEllipseScalar(asset, view, draw.trimStart, start) ||
            !resolveNativeEllipseScalar(asset, view, draw.trimEnd, end) ||
            !resolveNativeEllipseScalar(asset, view, draw.trimOffset, offset))
            return false;
        auto trimmed = trimPath(verbs, points, normalizeTrimSegment(start, end, offset));
        if (!trimmed.valid)
            return false;
        verbs = std::move(trimmed.verbs);
        points = std::move(trimmed.points);
    }
    return materializeSourcePath(verbs, points, {}, local) &&
           materializeSourcePath(verbs, points, transform, final);
}

bool sampleOwnScenePaint(const model::MotionAssetModel &asset, const OwnSceneDraw &draw,
                         const evaluation::PropertyEvaluationView &view,
                         runtime::EvaluatedStroke &stroke, runtime::EvaluatedPaint &paint) {
    if (!draw.paintColor.valid()) {
        const auto &record = asset.paints[asset.nodes[draw.node.index()].paint.index()];
        if (!record.staticValue)
            return false;
        stroke = record.staticValue->stroke;
        paint = record.staticValue->paint;
        return true;
    }
    const auto *color = asset.property(draw.paintColor);
    const auto *opacity = asset.property(draw.paintOpacity);
    if (!color || !opacity || color->staticValue.index >= asset.colorValues.size() ||
        opacity->staticValue.index >= asset.scalarValues.size())
        return false;
    const auto &c = asset.colorValues[color->staticValue.index];
    paint.kind = runtime::PaintKind::Solid;
    paint.solid = {static_cast<std::uint8_t>(255.0F * c.r), static_cast<std::uint8_t>(255.0F * c.g),
                   static_cast<std::uint8_t>(255.0F * c.b),
                   static_cast<std::uint8_t>(
                       255.0F * (asset.scalarValues[opacity->staticValue.index] / 100.0F))};
    if (draw.strokeWidth.valid()) {
        const auto *width = asset.property(draw.strokeWidth);
        const auto *source = asset.sourceNode(draw.paint);
        if (!width || !source ||
            !resolveNativeEllipseScalar(asset, view, draw.strokeWidth, stroke.width) ||
            !std::isfinite(stroke.width) || stroke.width < 0)
            return false;
        stroke.enabled = true;
        stroke.miterLimit = source->miterLimit;
        stroke.cap = source->strokeCap == model::SourceStrokeCap::Round ? runtime::LineCap::Round
                     : source->strokeCap == model::SourceStrokeCap::Square
                         ? runtime::LineCap::Square
                         : runtime::LineCap::Flat;
        stroke.join =
            source->strokeJoin == model::SourceStrokeJoin::Round   ? runtime::LineJoin::Round
            : source->strokeJoin == model::SourceStrokeJoin::Bevel ? runtime::LineJoin::Bevel
                                                                   : runtime::LineJoin::Miter;
    }
    return true;
}
} // namespace avemotion::render::detail
