#pragma once
#include "avemotion/evaluation/PropertyEvaluator.hpp"
#include <memory>
#include <optional>
#include <vector>

namespace avemotion::runtime::detail {
class OwnNativeEllipseModel;
class OwnVectorModel;
} // namespace avemotion::runtime::detail
namespace avemotion::render::detail {
struct OwnSceneLayer final {
    model::SourceNodeId source;
    model::LayerId layer;
    double inFrame = 0, outFrame = 0;
    model::IndexRange draws;
    bool precompDescendant = false;
};
struct OwnSceneDraw final {
    model::SourceNodeId group, path, paint;
    std::optional<model::SourceNodeId> trim;
    model::NodeId node;
    model::PropertyId shape, position, size, roundness, trimStart, trimEnd, trimOffset;
    model::PropertyId paintColor, paintOpacity, strokeWidth;
    runtime::FillRule fillRule = runtime::FillRule::Winding;
    model::ResourceClass geometryClass = model::ResourceClass::InstanceEvaluated;
    model::ResourceClass paintClass = model::ResourceClass::AssetStatic;
};
class OwnSceneProgram final {
  public:
    OwnSceneProgram(std::vector<OwnSceneLayer>, std::vector<OwnSceneDraw>);
    const std::vector<OwnSceneLayer> layers;
    const std::vector<OwnSceneDraw> draws;
};
std::shared_ptr<const OwnSceneProgram>
lowerOwnPrimitiveProgram(const runtime::detail::OwnNativeEllipseModel &,
                         const model::MotionAssetModel &);
std::shared_ptr<const OwnSceneProgram>
lowerOwnVectorProgram(const runtime::detail::OwnVectorModel &, model::MotionAssetModel &);
bool materializeOwnScenePath(const model::MotionAssetModel &, const OwnSceneDraw &,
                             const evaluation::PropertyEvaluationView &,
                             const runtime::AffineTransform &, runtime::EvaluatedPath &,
                             runtime::EvaluatedPath &);
bool sampleOwnScenePaint(const model::MotionAssetModel &, const OwnSceneDraw &,
                         runtime::EvaluatedStroke &, runtime::EvaluatedPaint &);
} // namespace avemotion::render::detail
