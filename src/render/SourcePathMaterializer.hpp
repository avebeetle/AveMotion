#pragma once
#include "avemotion/evaluation/PropertyEvaluator.hpp"
#include "avemotion/runtime/EvaluatedScene.hpp"
#include <span>
#include <vector>
namespace avemotion::render::detail {
struct ShapeSample final {
    std::span<const model::MotionVec2Value> points;
    bool closed = false;
    bool animated = false;
};
bool resolveShape(const model::MotionAssetModel &, const model::MotionPropertyRecord &,
                  const evaluation::PropertyEvaluationView &, ShapeSample &) noexcept;
bool shapePathStream(const ShapeSample &, std::vector<runtime::PathVerb> &,
                     std::vector<model::MotionVec2Value> &);
bool materializeSourcePath(std::span<const runtime::PathVerb>,
                           std::span<const model::MotionVec2Value>,
                           const runtime::AffineTransform &, runtime::EvaluatedPath &);
} // namespace avemotion::render::detail
