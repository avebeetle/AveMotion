#pragma once

#include "ExactRenderPlanComparison.hpp"
#include "OwnPrimitiveBinding.hpp"
#include "avemotion/render/HeadlessBackend.hpp"
#include "avemotion/runtime/RecordingBackend.hpp"

#include <string>
#include <algorithm>

namespace avemotion::test {

// Semantic normalization is confined to copies. Each original must first be
// checked against its own source binding and schema. Map only source roles
// (layer/group/primitive/fill and their bound properties), asset/instance cache
// identities, and literal versus escaped layer display labels. Remove both
// model pointers from comparison copies after checking resource counts, roles,
// content and aliases. Recompute copied scene fingerprints. For plans, map
// stamp/cache identities and attached scene copies; exclude only raw identity-
// dependent aggregate plan fingerprints. Keep revisions, updates, transforms,
// bounds, dirty flags, plan sequence and history. Original scenes feed planners.
class OwnPrimitiveSceneComparison final {
public:
    [[nodiscard]] std::string sceneDifference(
        const runtime::EvaluatedScene& reference,
        const runtime::detail::OwnPrimitiveBinding& referenceBinding,
        const runtime::EvaluatedScene& own,
        const runtime::detail::OwnPrimitiveBinding& ownBinding,
        std::uint64_t referenceId = 0) const {
        auto first = normalizedScene(reference, referenceBinding, false, referenceId);
        if (!first.error.empty()) return "reference " + first.error;
        auto second = normalizedScene(own, ownBinding, true, 0);
        if (!second.error.empty()) return "own " + second.error;
        return ExactSceneComparison{}.difference(first.scene, second.scene);
    }

    [[nodiscard]] std::string planDifference(
        const render::MotionRenderPlan& reference,
        const runtime::detail::OwnPrimitiveBinding& referenceBinding,
        const render::MotionRenderPlan& own,
        const runtime::detail::OwnPrimitiveBinding& ownBinding,
        std::uint64_t referenceId) const {
        auto first = normalizedPlan(reference, referenceBinding, false, referenceId);
        if (!first.error.empty()) return "reference " + first.error;
        auto second = normalizedPlan(own, ownBinding, true, 0);
        if (!second.error.empty()) return "own " + second.error;
        return ExactRenderPlanComparison{}.difference(first.plan, second.plan);
    }

private:
    struct NormalizedScene final {
        runtime::EvaluatedScene scene;
        std::string error;
    };
    struct NormalizedPlan final {
        render::MotionRenderPlan plan;
        std::string error;
    };

    [[nodiscard]] static bool equalPlanFingerprints(const render::RenderPlanFingerprints& a,
                                                     const render::RenderPlanFingerprints& b) {
        return a.plan == b.plan && a.topology == b.topology
            && a.geometryIdentity == b.geometryIdentity
            && a.paintIdentity == b.paintIdentity && a.presentation == b.presentation;
    }

    template <typename Key>
    static void normalizeKey(Key& key) {
        key.assetIdentity = 1;
        key.instanceId = key.scope == render::ResourceIdentityScope::Instance ? 1 : 0;
        key.instanceIdentity = key.scope == render::ResourceIdentityScope::Instance ? 1 : 0;
    }

    [[nodiscard]] static NormalizedPlan normalizedPlan(
        const render::MotionRenderPlan& original,
        const runtime::detail::OwnPrimitiveBinding& binding,
        bool own, std::uint64_t referenceId) {
        NormalizedPlan result{original, {}};
        if (!original.sourceScene) {
            result.error = "source scene absent";
            return result;
        }
        const auto& scene = *original.sourceScene;
        if (original.stamp.assetHash != scene.sourceAssetHash
            || original.stamp.instanceId != scene.instanceId
            || original.stamp.evaluationSequence != scene.evaluationSequence
            || original.stamp.assetIdentity != (scene.assetHandle.valid()
                ? scene.assetHandle.packed() : 0)
            || original.stamp.instanceIdentity != scene.instanceId) {
            result.error = "original stamp/source identity differs";
            return result;
        }
        if (!equalPlanFingerprints(original.fingerprints,
                                   render::computeRenderPlanFingerprints(original))) {
            result.error = "original plan fingerprints differ from recorder";
            return result;
        }
        for (const auto& item : original.drawItems) {
            if (item.sourceDrawItemIndex >= scene.drawItems.size()) {
                result.error = "original plan draw source index differs";
                return result;
            }
            const auto slot = item.sourceDrawItemIndex;
            const auto& geometry = item.geometry;
            if (geometry.assetHash != original.stamp.assetHash
                || geometry.assetIdentity != original.stamp.assetIdentity
                || geometry.instanceId != (geometry.scope == render::ResourceIdentityScope::Instance
                    ? original.stamp.instanceId : 0)
                || geometry.instanceIdentity != (geometry.scope == render::ResourceIdentityScope::Instance
                    ? original.stamp.instanceIdentity : 0)) {
                result.error = "original geometry cache identity differs";
                return result;
            }
            if (item.drawItem != model::makeId<model::DrawItemId>(slot)
                || item.node != model::makeId<model::NodeId>(slot)
                || geometry.resourceId != model::makeId<model::GeometryId>(slot)
                || geometry.sourceKey != slot + 1
                || geometry.scope != (scene.drawItems[item.sourceDrawItemIndex].geometryOrigin
                    == runtime::EvaluatedValueOrigin::AssetStatic
                    ? render::ResourceIdentityScope::Asset : render::ResourceIdentityScope::Instance)) {
                result.error = "original geometry plan role/class differs";
                return result;
            }
            const auto& key = item.paint;
            if (key.assetHash != original.stamp.assetHash
                || key.assetIdentity != original.stamp.assetIdentity
                || key.instanceId != (key.scope == render::ResourceIdentityScope::Instance
                    ? original.stamp.instanceId : 0)
                || key.instanceIdentity != (key.scope == render::ResourceIdentityScope::Instance
                    ? original.stamp.instanceIdentity : 0)) {
                result.error = "original paint cache identity differs";
                return result;
            }
            if (key.resourceId != model::makeId<model::PaintId>(slot)
                || key.sourceKey != slot + 1
                || key.scope != (scene.drawItems[item.sourceDrawItemIndex].paintOrigin
                    == runtime::EvaluatedValueOrigin::AssetStatic
                    ? render::ResourceIdentityScope::Asset : render::ResourceIdentityScope::Instance)) {
                result.error = "original paint plan role/class differs";
                return result;
            }
        }
        for (const auto& update : original.geometryUpdates) {
            if (update.planDrawItemIndex >= original.drawItems.size()
                || !(update.key == original.drawItems[update.planDrawItemIndex].geometry)) {
                result.error = "original geometry update/key differs";
                return result;
            }
        }
        for (const auto& update : original.paintUpdates) {
            if (update.planDrawItemIndex >= original.drawItems.size()
                || !(update.key == original.drawItems[update.planDrawItemIndex].paint)) {
                result.error = "original paint update/key differs";
                return result;
            }
        }
        auto normalized = normalizedScene(scene, binding, own, referenceId);
        if (!normalized.error.empty()) {
            result.error = "attached " + normalized.error;
            return result;
        }
        auto& plan = result.plan;
        plan.sourceScene = std::make_shared<const runtime::EvaluatedScene>(std::move(normalized.scene));
        plan.stamp.assetIdentity = 1;
        plan.stamp.instanceId = 1;
        plan.stamp.instanceIdentity = 1;
        for (auto& item : plan.drawItems) {
            normalizeKey(item.geometry);
            normalizeKey(item.paint);
        }
        for (auto& update : plan.geometryUpdates) normalizeKey(update.key);
        for (auto& update : plan.paintUpdates) normalizeKey(update.key);
        plan.fingerprints = render::computeRenderPlanFingerprints(plan);
        return result;
    }

    [[nodiscard]] static bool equalFingerprints(const runtime::SceneFingerprints& a,
                                                 const runtime::SceneFingerprints& b) {
        return a.scene == b.scene && a.topology == b.topology
            && a.geometry == b.geometry && a.paint == b.paint;
    }

    [[nodiscard]] static NormalizedScene normalizedScene(
        const runtime::EvaluatedScene& original,
        const runtime::detail::OwnPrimitiveBinding& binding,
        bool own, std::uint64_t referenceId) {
        NormalizedScene result{original, {}};
        auto& scene = result.scene;
        const auto fail = [&](std::string message) {
            result.error = std::move(message);
            return result;
        };
        if (!original.assetModel || !original.assetModelApplied
            || original.modelLayerCount != 2 || original.modelNodeCount != binding.groups.size()
            || original.modelGeometryCount != binding.groups.size() || original.modelPaintCount != binding.groups.size()
            || original.layers.size() != 2 || original.childLayerIndices != std::vector<std::uint32_t>{1}
            || (!original.drawItems.empty() && original.drawItems.size() != binding.groups.size()) || !original.masks.empty())
            return fail("model/resource schema differs");
        if (original.sourceAssetHash == 0 || original.assetModel->sourceAssetHash != original.sourceAssetHash)
            return fail("source hash differs from original model");
        if (own) {
            if (original.assetHandle.valid() || original.instanceHandle.valid()
                || original.instanceId == 0 || original.assetModel->assetHandle.valid())
                return fail("own original identity differs");
        } else if (original.assetHandle != runtime::AssetHandle{700001, 1}
                   || original.instanceHandle.valid() || original.instanceId != referenceId
                   || original.assetModel->assetHandle != runtime::AssetHandle{700001, 1}) {
            return fail("ordinary original identity differs");
        }
        if (!binding.root.valid() || !binding.layer.valid() || binding.groups.empty()
            || binding.groups.size() > 16 || binding.root == binding.layer)
            return fail("source role binding differs");
        const auto& model = *original.assetModel;
        if (model.layers.size() != 2 || model.nodes.size() != binding.groups.size()
            || model.geometries.size() != binding.groups.size() || model.paints.size() != binding.groups.size())
            return fail("original model resource counts differ");
        if (binding.root.index() >= model.sourceNodes.size()
            || binding.layer.index() >= model.sourceNodes.size()
            || original.layers[0].modelLayer != model::makeId<model::LayerId>(0)
            || original.layers[1].modelLayer != model::makeId<model::LayerId>(1))
            return fail("source/model role indices differ");
        std::vector<bool> roles(model.sourceNodes.size());
        roles[binding.root.index()] = roles[binding.layer.index()] = true;
        for (const auto& group : binding.groups) {
            for (const auto id : {group.group, group.primitive, group.fill}) {
                if (!id.valid() || id.index() >= roles.size() || roles[id.index()])
                    return fail("source group roles are not bijective");
                roles[id.index()] = true;
            }
        }
        if (std::find(roles.begin(), roles.end(), false) != roles.end())
            return fail("unconsumed source role");
        if (original.layers[0].keyPath != model.layers[0].debugName)
            return fail("original root display label differs");
        if (original.layers[1].keyPath != model.layers[1].debugName)
            return fail("original shape display label differs");
        for (std::size_t slot = 0; slot < original.drawItems.size(); ++slot) {
            const auto& item = original.drawItems[slot];
            const auto& group = binding.groups[binding.groups.size() - 1 - slot];
            if (item.sourcePathNode != group.primitive || item.sourcePaintNode != group.fill
                || item.modelDrawItem != model::makeId<model::DrawItemId>(slot)
                || item.modelNode != model::makeId<model::NodeId>(slot)
                || item.modelGeometry != model::makeId<model::GeometryId>(slot)
                || item.modelPaint != model::makeId<model::PaintId>(slot)
                || item.drawOrder != slot || item.sourcePathCount != 1 || !item.sourcePathModifierFree)
                return fail("original draw/source role differs");
            const auto* geometry = model.geometry(item.modelGeometry);
            const auto* paint = model.paint(item.modelPaint);
            if (!geometry || !paint || item.sourceGeometryId != slot + 1
                || item.sourcePaintId != slot + 1)
                return fail("original resource source IDs differ");
            const bool staticGeometry = geometry->resourceClass == model::ResourceClass::AssetStatic
                && geometry->staticValue && item.localGeometryAvailable && item.localPaintAvailable;
            const bool staticPaint = paint->resourceClass == model::ResourceClass::AssetStatic
                && paint->staticValue && item.localPaintAvailable;
            if (bool(item.canonicalGeometry) != staticGeometry
                || bool(item.canonicalPaint) != staticPaint
                || item.geometryOrigin != (staticGeometry
                    ? runtime::EvaluatedValueOrigin::AssetStatic
                    : runtime::EvaluatedValueOrigin::InstanceEvaluated)
                || item.paintOrigin != (staticPaint
                    ? runtime::EvaluatedValueOrigin::AssetStatic
                    : runtime::EvaluatedValueOrigin::InstanceEvaluated))
                return fail("original resource class/alias presence differs");
            if (item.canonicalGeometry) {
                if (item.canonicalGeometry.get() != &*geometry->staticValue
                    || item.canonicalGeometry.owner_before(original.assetModel)
                    || original.assetModel.owner_before(item.canonicalGeometry))
                    return fail("original canonical geometry alias differs");
            }
            if (item.canonicalPaint) {
                if (item.canonicalPaint.get() != &*paint->staticValue
                    || item.canonicalPaint.owner_before(original.assetModel)
                    || original.assetModel.owner_before(item.canonicalPaint))
                    return fail("original canonical paint alias differs");
            }
        }
        if (!equalFingerprints(original.fingerprints, runtime::computeSceneFingerprints(original)))
            return fail("original scene fingerprints differ from recorder");
        scene.assetHandle = {};
        scene.instanceHandle = {};
        scene.instanceId = 1;
        scene.layers[0].keyPath = "<root>";
        scene.layers[1].keyPath = "<shape>";
        for (std::size_t slot = 0; slot < scene.drawItems.size(); ++slot) {
            auto& item = scene.drawItems[slot];
            const auto authored = binding.groups.size() - 1 - slot;
            item.sourcePathNode = model::makeId<model::SourceNodeId>(3 + 3 * authored);
            item.sourcePaintNode = model::makeId<model::SourceNodeId>(4 + 3 * authored);
        }
        scene.assetModel.reset();
        scene.fingerprints = runtime::computeSceneFingerprints(scene);
        return result;
    }
};

} // namespace avemotion::test
