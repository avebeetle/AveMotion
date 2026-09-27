#pragma once
#include "NativeEllipseCertificate.hpp"
#include "NativeEllipseOracle.hpp"
#include "OwnVectorTestData.hpp"
#include "lottieitem.h"
#include <cmath>
#include <iostream>
#include <map>
#include <set>

namespace avemotion::test {
class OwnVectorSceneOracle final {
  public:
    explicit OwnVectorSceneOracle(const std::string &json) : json_(json) {
        try {
            ordinary_ = std::make_unique<NativeEllipseOracle>(json);
            model_ = ordinary_->model();
            std::cout << "oracle route: ordinary finalized model\n";
        } catch (const std::exception &e) {
            std::cout << "ordinary finalization unavailable: " << e.what() << '\n';
            auto animation =
                rlottie::Animation::loadFromData(json, "vector-oracle-metadata", {}, false);
            vectorRequire(bool(animation), "ordinary oracle parse");
            std::size_t w, h;
            animation->size(w, h);
            sourceHash_ = core::fnv1a64(std::as_bytes(std::span(json.data(), json.size())));
            auto extracted =
                model::detail::evaluateTelegramParsedProperties(json, "vector-oracle-source",
                                                                {{99, 1},
                                                                 sourceHash_,
                                                                 w,
                                                                 h,
                                                                 animation->frameRate(),
                                                                 animation->totalFrame(),
                                                                 "vector-oracle"},
                                                                0);
            vectorRequire(bool(extracted), "ordinary source roles: " + extracted.error);
            model_ = extracted.model;
            std::cout << "oracle route: fresh ordinary bridge; parsed model for "
                         "roles only\n";
        }
        auto matrixSeed = rlottie::Animation::loadFromData(json, "vector-matrix-source", {}, false);
        vectorRequire(bool(matrixSeed), "independent matrix source parse");
        matrixSource_ = rlottie::AveMotionAnimationAccess::model(*matrixSeed);
        auto matrixModel =
            model::detail::buildTelegramParsedModel(matrixSource_, {{101, 1},
                                                                    model_->sourceAssetHash,
                                                                    model_->logicalWidth,
                                                                    model_->logicalHeight,
                                                                    model_->frameRate,
                                                                    model_->totalFrames,
                                                                    "matrix-oracle"});
        vectorRequire(bool(matrixModel), "matrix source binding: " + matrixModel.error);
    }
    runtime::EvaluatedScene freshScene(std::size_t frame, std::size_t width, std::size_t height) {
        runtime::EvaluatedScene scene;
        if (ordinary_)
            scene = ordinary_->freshScene(frame, width, height);
        else {
            auto animation = rlottie::Animation::loadFromData(
                json_, "vector-oracle-fresh-" + std::to_string(++sequence_), {}, false);
            vectorRequire(bool(animation), "fresh ordinary parse");
            auto built = runtime::detail::buildSceneFromRlottieTree(
                animation->renderTree(frame, width, height), sourceHash_, 0, sequence_, frame,
                width, height);
            vectorRequire(bool(built), "ordinary bridge: " + built.error.message);
            scene = std::move(built.scene);
        }
        scene.instanceId = 0x26A26004ULL;
        assertCanvasClip(scene);
        auto root = LOTCompItem::createLayerItem(matrixSource_->mRoot->mRootLayer.get());
        vectorRequire(bool(root), "fresh matrix-only layer tree");
        root->setComplexContent(false);
        AveSourceIdState ids;
        root->assignAveSourceIds(ids);
        root->resetForRecording();
        const float scale =
            std::min(static_cast<float>(width) / static_cast<float>(model_->logicalWidth),
                     static_cast<float>(height) / static_cast<float>(model_->logicalHeight));
        VMatrix viewport;
        viewport
            .translate(
                (static_cast<float>(width) - static_cast<float>(model_->logicalWidth) * scale) *
                    0.5F,
                (static_cast<float>(height) - static_cast<float>(model_->logicalHeight) * scale) *
                    0.5F)
            .scale(scale, scale);
        root->update(static_cast<int>(frame), viewport, 1.0F);
        std::vector<VDrawable *> drawables;
        root->renderList(drawables);
        matrices_.clear();
        for (auto *value : drawables) {
            const auto *drawable = static_cast<const LOTDrawable *>(value);
            vectorRequire(
                matrices_.emplace(drawable->mAveSourcePaintNodeId, drawable->mAveLocalTransform)
                    .second,
                "unique matrix source paint role");
        }
        vectorRequire(matrices_.size() == scene.drawItems.size(),
                      "matrix role bijection covers every visible draw");
        for (const auto &item : scene.drawItems) {
            const auto found = matrices_.find(item.sourcePaintNode.value);
            vectorRequire(found != matrices_.end(), "ordinary matrix role exists");
            const auto &m = found->second;
            const auto &a = item.localToViewport;
            vectorRequire(m.m_11() == a.m11 && m.m_12() == a.m12 && m.m_21() == a.m21 &&
                              m.m_22() == a.m22 && m.m_tx() == a.dx && m.m_ty() == a.dy,
                          "pinned matrix matches bridge coefficients exactly");
        }
        return scene;
    }
    const model::MotionAssetModel &model() const { return *model_; }
    const std::map<std::uint32_t, VMatrix> &matrices() const { return matrices_; }
    void assertCanvasClip(const runtime::EvaluatedScene &scene) const {
        vectorRequire(scene.masks.empty(), "ordinary masks absent");
        for (const auto &layer : scene.layers) {
            vectorRequire(layer.matte == runtime::MatteMode::None && layer.maskCount == 0,
                          "ordinary matte/mask absent");
            if (layer.clipPath.verbs.empty())
                continue;
            const auto &p = layer.clipPath;
            vectorRequire(layer.drawItemCount == 0 && layer.childCount > 0 && layer.opacity == 1,
                          "clip belongs to full-opacity structural ancestor");
            vectorRequire(p.verbs ==
                              std::vector<runtime::PathVerb>{
                                  runtime::PathVerb::MoveTo, runtime::PathVerb::LineTo,
                                  runtime::PathVerb::LineTo, runtime::PathVerb::LineTo,
                                  runtime::PathVerb::LineTo, runtime::PathVerb::Close},
                          "full-canvas rectangular clip topology");
            vectorRequire(p.points.size() == 5, "canvas clip has four corners and repeated start");
            const auto w = static_cast<float>(scene.viewportWidth),
                       h = static_cast<float>(scene.viewportHeight);
            const float scale = std::min(w / static_cast<float>(model_->logicalWidth),
                                         h / static_cast<float>(model_->logicalHeight));
            const float left = (w - static_cast<float>(model_->logicalWidth) * scale) * 0.5F;
            const float top = (h - static_cast<float>(model_->logicalHeight) * scale) * 0.5F;
            const float right = static_cast<float>(model_->logicalWidth) * scale + left;
            const float bottom = static_cast<float>(model_->logicalHeight) * scale + top;
            const std::set<std::pair<float, float>> corners{
                {left, top}, {right, top}, {right, bottom}, {left, bottom}};
            std::set<std::pair<float, float>> got;
            for (const auto point : p.points)
                got.emplace(point.x, point.y);
            vectorRequire(got == corners, "identity canvas clip equals mapped composition canvas");
            const std::vector<runtime::Vec2> expected{
                {right, top}, {right, bottom}, {left, bottom}, {left, top}, {right, top}};
            for (std::size_t i = 0; i < expected.size(); ++i)
                vectorRequire(p.points[i].x == expected[i].x && p.points[i].y == expected[i].y,
                              "ordered identity canvas rectangle");
        }
    }

  private:
    std::string json_;
    std::unique_ptr<NativeEllipseOracle> ordinary_;
    std::shared_ptr<const model::MotionAssetModel> model_;
    std::shared_ptr<LOTModel> matrixSource_;
    std::map<std::uint32_t, VMatrix> matrices_;
    std::uint64_t sourceHash_ = 0, sequence_ = 0;
};
} // namespace avemotion::test
