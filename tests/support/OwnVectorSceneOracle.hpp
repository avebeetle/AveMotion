#pragma once
#include "NativeEllipseCertificate.hpp"
#include "NativeEllipseOracle.hpp"
#include "OwnVectorTestData.hpp"
#include "lottieitem.h"
#include <cmath>
#include <functional>
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
        buildInstances(matrixSource_->mRoot->mRootLayer.get(), {}, "root", "root", 0, 0);
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
        validateInstances(scene);
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
                matrices_.emplace(drawable->mAveNodeId, drawable->mAveLocalTransform).second,
                "unique matrix source paint role");
        }
        vectorRequire(matrices_.size() == scene.drawItems.size(),
                      "matrix role bijection covers every visible draw");
        for (const auto &item : scene.drawItems) {
            const auto found = matrices_.find(item.modelNode.value);
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
    struct Instance {
        const LOTLayerData *data;
        std::uint32_t parent;
        std::string path, composition;
        int offset;
    };
    const std::map<std::uint32_t, Instance> &instances() const { return instances_; }
    bool active(std::uint32_t id, int frame) const {
        const auto &instance = instances_.at(id);
        const auto local = frame - instance.offset;
        return local >= instance.data->inFrame() && local < instance.data->outFrame() &&
               std::abs(instance.data->opacity(local)) > 0.000001F &&
               (instance.parent == runtime::kInvalidSceneIndex || active(instance.parent, frame));
    }
    std::string definitionRole(model::SourceNodeId id) const {
        const auto &n = model_->sourceNodes.at(id.index());
        if (n.kind == model::SourceNodeKind::Layer)
            return (n.composition.index() == 0
                        ? "root"
                        : model_->compositions.at(n.composition.index()).debugName) +
                   "/layer:" + std::to_string(n.authoredLayerId);
        const auto &p = model_->sourceNodes.at(n.parent.index());
        const auto children =
            std::span(model_->sourceChildIds).subspan(p.children.first, p.children.count);
        const auto found = std::find(children.begin(), children.end(), id);
        vectorRequire(found != children.end(), "definition child belongs to parent");
        return definitionRole(p.id) +
               (p.kind == model::SourceNodeKind::Layer ? "/shapes/" : "/it/") +
               std::to_string(found - children.begin());
    }
    std::string drawRole(const runtime::EvaluatedScene &scene,
                         const runtime::EvaluatedDrawItem &draw) const {
        const auto &instance = instances_.at(scene.layers.at(draw.layerIndex).modelLayer.value);
        vectorRequire(instance.data->mLayerType == LayerType::Shape, "draw owner is shape layer");
        const auto prefix = instance.composition + "/layer:" + std::to_string(instance.data->id());
        const auto path = definitionRole(draw.sourcePathNode),
                   paint = definitionRole(draw.sourcePaintNode);
        vectorRequire(path.starts_with(prefix + "/") && paint.starts_with(prefix + "/"),
                      "draw definitions belong to occurrence");
        return instance.path + "|" + path + "|" + paint;
    }
    VMatrix world(const Instance &instance, int frame, const VMatrix &viewport) const {
        std::function<VMatrix(const LOTLayerData *, std::set<int>)> local =
            [&](const LOTLayerData *data, std::set<int> seen) {
                vectorRequire(seen.insert(data->id()).second, "reference transform parent cycle");
                auto result = data->matrix(frame - instance.offset);
                if (data->parentId() >= 0) {
                    const auto &siblings = instances_.at(instance.parent).data->mChildren;
                    const LOTLayerData *parent = nullptr;
                    for (const auto &entry : siblings) {
                        const auto *candidate = static_cast<const LOTLayerData *>(entry.get());
                        if (candidate->id() == data->parentId())
                            parent = candidate;
                    }
                    vectorRequire(parent != nullptr, "reference transform parent exists");
                    result *= local(parent, seen);
                }
                return result;
            };
        return local(instance.data, {}) *
               (instance.parent == runtime::kInvalidSceneIndex
                    ? viewport
                    : world(instances_.at(instance.parent), frame, viewport));
    }
    VMatrix viewport(const runtime::EvaluatedScene &scene) const {
        const float scale = std::min(float(scene.viewportWidth) / float(model_->logicalWidth),
                                     float(scene.viewportHeight) / float(model_->logicalHeight));
        VMatrix result;
        result
            .translate((float(scene.viewportWidth) - float(model_->logicalWidth) * scale) * .5F,
                       (float(scene.viewportHeight) - float(model_->logicalHeight) * scale) * .5F)
            .scale(scale, scale);
        return result;
    }
    VMatrix sourceWorld(const runtime::EvaluatedScene &scene,
                        const runtime::EvaluatedDrawItem &draw, model::SourceNodeId source) const {
        const auto &instance = instances_.at(scene.layers.at(draw.layerIndex).modelLayer.value);
        const auto &node = model_->sourceNodes.at(source.index());
        auto result = world(instance, int(scene.frameIndex), viewport(scene));
        const auto &parent = model_->sourceNodes.at(node.parent.index());
        if (parent.kind == model::SourceNodeKind::ShapeGroup) {
            const auto *group = groups_.at(parent.id.value);
            if (group->mTransform)
                result =
                    group->mTransform->matrix(int(scene.frameIndex) - instance.offset) * result;
        } else
            vectorRequire(parent.kind == model::SourceNodeKind::Layer, "bounded source scope");
        return result;
    }
    void validateInstances(const runtime::EvaluatedScene &scene) const {
        vectorRequire(scene.layers.size() == instances_.size(),
                      "full reference instance map coverage");
        std::set<std::uint32_t> seen;
        for (const auto &layer : scene.layers) {
            vectorRequire(seen.insert(layer.modelLayer.value).second,
                          "unique reference execution layer");
            const auto &instance = instances_.at(layer.modelLayer.value);
            const auto parent = layer.parentLayer == runtime::kInvalidSceneIndex
                                    ? runtime::kInvalidSceneIndex
                                    : scene.layers.at(layer.parentLayer).modelLayer.value;
            vectorRequire(parent == instance.parent, "reference structural edge");
            int localFrame = int(scene.frameIndex) - instance.offset;
            if (parent != runtime::kInvalidSceneIndex) {
                if (!active(parent, int(scene.frameIndex)))
                    localFrame = -1;
            }
            vectorRequire(layer.visible == (localFrame >= instance.data->inFrame() &&
                                            localFrame < instance.data->outFrame()),
                          "reference local clock visibility " + instance.path);
            std::set<std::uint32_t> children;
            for (std::uint32_t i = 0; i < layer.childCount; ++i)
                children.insert(
                    scene.layers.at(scene.childLayerIndices.at(layer.firstChildReference + i))
                        .modelLayer.value);
            std::set<std::uint32_t> expected;
            for (const auto &[id, child] : instances_)
                if (child.parent == layer.modelLayer.value)
                    expected.insert(id);
            vectorRequire(children == expected, "reference complete child edges");
        }
        std::set<std::uint32_t> drawIds;
        for (const auto &draw : scene.drawItems) {
            vectorRequire(drawIds.insert(draw.modelNode.value).second,
                          "unique reference execution draw ID");
            const auto id = scene.layers.at(draw.layerIndex).modelLayer.value;
            vectorRequire(active(id, int(scene.frameIndex)),
                          "reference draw has active structural ancestry");
        }
    }
    void assertCanvasClip(const runtime::EvaluatedScene &scene) const {
        vectorRequire(scene.masks.empty(), "ordinary masks absent");
        for (const auto &layer : scene.layers) {
            vectorRequire(layer.matte == runtime::MatteMode::None && layer.maskCount == 0,
                          "ordinary matte/mask absent");
            const auto &instance = instances_.at(layer.modelLayer.value);
            const bool expectedClip = instance.data->precompLayer() &&
                                      !instance.data->layerSize().empty() &&
                                      active(layer.modelLayer.value, int(scene.frameIndex));
            vectorRequire(expectedClip == !layer.clipPath.verbs.empty(),
                          "required reference clip presence " + instance.path);
            if (!expectedClip)
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
            const auto matrix = world(instance, int(scene.frameIndex), viewport(scene));
            const auto first = matrix.map(0, 0),
                       last = matrix.map(float(instance.data->layerSize().width()),
                                         float(instance.data->layerSize().height()));
            const float left = first.x(), top = first.y(), right = last.x(), bottom = last.y();
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
    void assertRedundantClips(const runtime::EvaluatedScene &scene) const {
        assertCanvasClip(scene);
        for (const auto &draw : scene.drawItems) {
            if (draw.path.points.empty())
                continue;
            float left = draw.path.points.front().x, right = left;
            float top = draw.path.points.front().y, bottom = top;
            for (const auto p : draw.path.points) {
                left = std::min(left, p.x);
                right = std::max(right, p.x);
                top = std::min(top, p.y);
                bottom = std::max(bottom, p.y);
            }
            auto parent = instances_.at(scene.layers.at(draw.layerIndex).modelLayer.value).parent;
            for (; parent != runtime::kInvalidSceneIndex; parent = instances_.at(parent).parent) {
                const auto &ancestor = instances_.at(parent);
                if (ancestor.data->layerSize().empty())
                    continue;
                const auto transform = world(ancestor, int(scene.frameIndex), viewport(scene));
                const auto low = transform.map(0, 0),
                           high = transform.map(float(ancestor.data->layerSize().width()),
                                                float(ancestor.data->layerSize().height()));
                double pen = draw.stroke.enabled ? double(draw.stroke.width) : 0;
                if (draw.stroke.enabled && draw.stroke.join == runtime::LineJoin::Miter)
                    pen = std::max(pen, .5 * draw.stroke.width * (draw.stroke.miterLimit + 1));
                const double magnitude =
                    std::max({1., std::abs(double(left)), std::abs(double(right)),
                              std::abs(double(top)), std::abs(double(bottom)),
                              std::abs(double(high.x())), std::abs(double(high.y()))});
                const double padding =
                    pen + 2 + 16 * std::numeric_limits<float>::epsilon() * magnitude;
                const double l = std::max(0., left - padding), t = std::max(0., top - padding);
                const double r = std::min(double(scene.viewportWidth), right + padding),
                             b = std::min(double(scene.viewportHeight), bottom + padding);
                vectorRequire(l >= r || t >= b ||
                                  (l >= low.x() && t >= low.y() && r <= high.x() && b <= high.y()),
                              "independent raster hull contained in every reference clip " +
                                  ancestor.path);
            }
        }
    }

  private:
    void buildInstances(const LOTLayerData *data, std::set<std::string> stack, std::string path,
                        std::string composition, int offset, std::uint32_t parent) {
        const auto id = static_cast<std::uint32_t>(instances_.size());
        instances_.emplace(id, Instance{data, id == 0 ? runtime::kInvalidSceneIndex : parent, path,
                                        composition, offset});
        std::function<void(const LOTGroupData *)> groups = [&](const LOTGroupData *group) {
            if (group->type() == LOTData::Type::ShapeGroup)
                groups_[group->aveMotionSourceNodeId()] = group;
            for (const auto &child : group->mChildren)
                if (child->type() == LOTData::Type::ShapeGroup)
                    groups(static_cast<const LOTGroupData *>(child.get()));
        };
        if (!data->precompLayer()) {
            groups(data);
            return;
        }
        if (id != 0) {
            vectorRequire(data->mExtra && !data->mExtra->mPreCompRefId.empty(),
                          "reference precomp ref exists");
            composition = data->mExtra->mPreCompRefId;
            vectorRequire(stack.insert(composition).second, "reference asset cycle");
            const auto asset = matrixSource_->mRoot->mAssets.find(composition);
            vectorRequire(asset != matrixSource_->mRoot->mAssets.end() && asset->second &&
                              asset->second->mLayers == data->mChildren,
                          "reference instance uses complete referenced asset");
            path += "->" + composition;
            offset += data->startFrame();
        }
        std::set<int> authored;
        for (std::size_t i = data->mChildren.size(); i-- > 0;) {
            const auto *child = static_cast<const LOTLayerData *>(data->mChildren[i].get());
            vectorRequire(authored.insert(child->id()).second, "reference scoped layer identity");
            buildInstances(child, stack,
                           path + "/layer:" + std::to_string(child->id()) + "[" +
                               std::to_string(i) + "]",
                           composition, offset, id);
        }
    }
    std::map<std::uint32_t, Instance> instances_;
    std::map<std::uint32_t, const LOTGroupData *> groups_;
    std::string json_;
    std::unique_ptr<NativeEllipseOracle> ordinary_;
    std::shared_ptr<const model::MotionAssetModel> model_;
    std::shared_ptr<LOTModel> matrixSource_;
    std::map<std::uint32_t, VMatrix> matrices_;
    std::uint64_t sourceHash_ = 0, sequence_ = 0;
};
} // namespace avemotion::test
