#include "NativeEllipseCertificate.hpp"
#include "OwnNativeEllipseStream.hpp"
#include "OwnNativeEllipseModelTestData.hpp"
#include "OwnNativeEllipseStreamTestData.hpp"
#include "support/NativeEllipseOracle.hpp"
#include "support/NativeEllipseTestAssets.hpp"
#include "support/OwnNativeEllipseStreamComparison.hpp"
#include "avemotion/render/RenderPlanner.hpp"

#include <algorithm>
#include <array>
#include <fstream>
#include <iostream>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <string>

namespace {
using namespace avemotion;
constexpr std::uint64_t kFixedOrdinaryPlanId = 0x26A26002ULL;

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

struct History final {
    runtime::SceneFingerprints previous;
    bool hasPrevious = false;
};

void checkHistory(const runtime::EvaluatedScene& scene, History& history,
                  const std::string& label) {
    const auto& changes = scene.changes;
    require(changes.firstEvaluation == !history.hasPrevious, label + " first evaluation");
    if (history.hasPrevious) {
        require(changes.topologyChanged == (scene.fingerprints.topology != history.previous.topology)
            && changes.geometryChanged == (scene.fingerprints.geometry != history.previous.geometry)
            && changes.paintChanged == (scene.fingerprints.paint != history.previous.paint)
            && changes.visualChanged == (scene.fingerprints.scene != history.previous.scene),
            label + " successful history");
    }
    history.previous = scene.fingerprints;
    history.hasPrevious = true;
}

struct SequenceContext final {
    test::NativeEllipseOracle& oracle;
    render::detail::OwnNativeEllipseStream& stream;
    const runtime::detail::NativeEllipseModelBinding& referenceBinding;
    const runtime::detail::NativeEllipseModelBinding& ownBinding;
    render::MotionRenderPlanner referencePlanner;
    render::MotionRenderPlanner ownPlanner;
    History referenceHistory, ownHistory;
    std::uint64_t sourceHash = 0;
    std::uint64_t ownInstanceId = 0;
    std::uint64_t ordinal = 0;
    std::size_t comparisons = 0;
};

void compareFrame(SequenceContext& context, const std::string& caseName,
                  const std::string& order, std::size_t frame,
                  std::size_t width, std::size_t height) {
    const auto label = caseName + " " + order + " frame=" + std::to_string(frame)
        + " viewport=" + std::to_string(width) + "x" + std::to_string(height)
        + " ordinal=" + std::to_string(context.ordinal + 1);
    auto reference = context.oracle.freshScene(frame, width, height);
    require(reference.instanceId == 0 && reference.assetHandle == runtime::AssetHandle{700001, 1}
        && reference.assetModel.get() == context.oracle.model().get(),
        label + " untouched ordinary oracle identity/model");
    const auto emitted = context.stream.emit(frame, width, height);
    require(bool(emitted), label + " own emission: " + emitted.message);
    auto own = *emitted.scene;
    if (context.ordinal == 0) context.ownInstanceId = own.instanceId;
    ++context.ordinal;
    require(reference.evaluationSequence == context.ordinal
        && own.evaluationSequence == context.ordinal
        && reference.sourceAssetHash == context.sourceHash
        && own.sourceAssetHash == context.sourceHash
        && own.instanceId == context.ownInstanceId && own.instanceId != 0
        && reference.frameIndex == std::min<std::size_t>(frame, 60)
        && own.frameIndex == reference.frameIndex,
        label + " original sequence and frame");
    checkHistory(reference, context.referenceHistory, label + " ordinary");
    checkHistory(own, context.ownHistory, label + " own");
    reference.instanceId = kFixedOrdinaryPlanId;
    const test::OwnNativeEllipseStreamComparison comparator;
    const auto sceneDifference = comparator.sceneDifference(
        reference, context.referenceBinding, own, context.ownBinding, kFixedOrdinaryPlanId);
    require(sceneDifference.empty(), label + " scene: " + sceneDifference);
    const auto referencePlan = context.referencePlanner.build(
        std::make_shared<const runtime::EvaluatedScene>(reference));
    const auto ownPlan = context.ownPlanner.build(
        std::make_shared<const runtime::EvaluatedScene>(own));
    require(bool(referencePlan), label + " ordinary plan: " + referencePlan.error.message);
    require(bool(ownPlan), label + " own plan: " + ownPlan.error.message);
    const auto planDifference = comparator.planDifference(referencePlan.plan,
        context.referenceBinding, ownPlan.plan, context.ownBinding, kFixedOrdinaryPlanId);
    require(planDifference.empty(), label + " plan: " + planDifference);
    ++context.comparisons;
}

std::size_t mutationWitnesses(const std::string& json) {
    const auto decoded = runtime::detail::decodeNativeEllipseInput(json);
    require(bool(decoded), "witness ordinary input");
    const auto owner = test::prepareOwnEllipseForTest(json);
    test::NativeEllipseOracle oracle(json);
    const auto bound = runtime::detail::bindNativeEllipseModel(*decoded.input, *oracle.model());
    require(bool(bound), "witness ordinary binding");
    auto stream = render::detail::OwnNativeEllipseStream::create(owner);
    require(bool(stream), "witness own stream");
    auto reference = oracle.freshScene(30, 512, 512);
    reference.instanceId = kFixedOrdinaryPlanId;
    const auto emitted = stream.stream->emit(30, 512, 512);
    require(bool(emitted), "witness own scene");
    const auto& own = *emitted.scene;
    require(own.drawItems.size() == 1 && !own.drawItems[0].localPath.points.empty(),
        "witness visible geometry");
    const test::OwnNativeEllipseStreamComparison comparator;
    require(comparator.sceneDifference(reference, *bound.binding, own, owner->authored->binding,
        kFixedOrdinaryPlanId).empty(), "witness scene baseline");
    std::size_t witnesses = 0;
    const auto witnessScene = [&](const char* name, auto mutate) {
        auto changed = own;
        mutate(changed);
        changed.fingerprints = runtime::computeSceneFingerprints(changed);
        const auto difference = comparator.sceneDifference(reference, *bound.binding,
            changed, owner->authored->binding, kFixedOrdinaryPlanId);
        require(!difference.empty(), std::string{name} + " mutation escaped scene comparator");
        std::cout << "WITNESS " << name << " rejected=" << difference << '\n';
        ++witnesses;
    };
    witnessScene("geometry", [](auto& scene) { scene.drawItems[0].localPath.points[0].x += 1; });
    witnessScene("color", [](auto& scene) { scene.drawItems[0].paint.solid.r ^= 1; });
    witnessScene("transform", [](auto& scene) { scene.drawItems[0].localToViewport.dx += 1; });
    witnessScene("visibility", [](auto& scene) { scene.layers[1].visible = false; });
    witnessScene("unused-gradient", [](auto& scene) {
        scene.drawItems[0].paint.gradient.centerRadius = 1;
    });
    witnessScene("unused-image", [](auto& scene) {
        scene.drawItems[0].paint.image.matrix[0] = 1;
    });
    witnessScene("unused-stroke", [](auto& scene) {
        scene.drawItems[0].stroke.miterLimit = 2;
    });
    witnessScene("scene-bounds", [](auto& scene) { scene.controlBounds.right += 1; });
    witnessScene("wrong-source-role", [&](auto& scene) {
        scene.drawItems[0].sourcePathNode = owner->authored->binding.group;
    });

    render::MotionRenderPlanner referencePlanner, ownPlanner;
    const auto referencePlan = referencePlanner.build(
        std::make_shared<const runtime::EvaluatedScene>(reference));
    const auto ownPlan = ownPlanner.build(std::make_shared<const runtime::EvaluatedScene>(own));
    require(bool(referencePlan) && bool(ownPlan), "witness plans built");
    require(comparator.planDifference(referencePlan.plan, *bound.binding, ownPlan.plan,
        owner->authored->binding, kFixedOrdinaryPlanId).empty(), "witness plan baseline");
    const auto witnessPlan = [&](const char* name, auto mutate) {
        auto changed = ownPlan.plan;
        mutate(changed);
        changed.fingerprints = render::computeRenderPlanFingerprints(changed);
        const auto difference = comparator.planDifference(referencePlan.plan, *bound.binding,
            changed, owner->authored->binding, kFixedOrdinaryPlanId);
        require(!difference.empty(), std::string{name} + " mutation escaped plan comparator");
        std::cout << "WITNESS " << name << " rejected=" << difference << '\n';
        ++witnesses;
    };
    witnessPlan("resource-revision", [](auto& plan) {
        ++plan.drawItems[0].geometry.revision;
        for (auto& update : plan.geometryUpdates)
            update.key.revision = plan.drawItems[0].geometry.revision;
    });
    witnessPlan("plan-bounds", [](auto& plan) { plan.presentedBounds.right += 1; });
    witnessPlan("dirty-region", [](auto& plan) { plan.dirtyRegion.right += 1; });
    return witnesses;
}

void checkOwnOnlyRuntimeCounters(const std::string& json) {
    runtime::Runtime liveRuntime;
    const auto before = liveRuntime.diagnostics();
    auto stream = render::detail::OwnNativeEllipseStream::create(
        test::prepareOwnEllipseForTest(json));
    require(bool(stream), "measured own stream");
    for (std::size_t frame = 0; frame <= 60; ++frame)
        require(bool(stream.stream->emit(frame, 512, 512)), "measured own emission");
    const auto after = liveRuntime.diagnostics();
    require(before.referenceMetadataSessionsCreated == after.referenceMetadataSessionsCreated
        && before.referenceSceneSessionsCreated == after.referenceSceneSessionsCreated
        && before.referenceModelSessionsCreated == after.referenceModelSessionsCreated
        && before.referenceCpuSessionsCreated == after.referenceCpuSessionsCreated
        && before.referenceSceneSamples == after.referenceSceneSamples
        && before.referenceModelSamples == after.referenceModelSamples
        && before.sceneEvaluations == after.sceneEvaluations
        && before.modelEvaluations == after.modelEvaluations
        && before.cpuFramesRendered == after.cpuFramesRendered,
        "own-only calls leave live Runtime reference counters unchanged");
    std::cout << "COUNTERS ownOnlyFrames=61 liveReferenceDeltas=0\n";
}
} // namespace

int main() {
    try {
        std::ifstream input(std::string(AVEMOTION_FIXTURE_DIR) + "/telegram_sticker_basic.json");
        require(bool(input), "baseline fixture readable");
        const std::string json(std::istreambuf_iterator<char>{input}, {});
        checkOwnOnlyRuntimeCounters(json);
        const auto witnesses = mutationWitnesses(json);
        require(witnesses == 12, "mutation witness count");
        const std::array<std::pair<std::size_t, std::size_t>, 4> viewports{{
            {512, 512}, {256, 256}, {384, 256}, {256, 384}}};
        const std::array<std::size_t, 8> seeks{0, 30, 30, 60, 10, 20, 19, 0};
        std::size_t comparisons = 0, directSamples = 0, directParses = 0;
        for (const auto& testAsset : test::nativeEllipseTestAssets(json)) {
            const auto decoded = runtime::detail::decodeNativeEllipseInput(testAsset.json);
            require(bool(decoded), testAsset.name + " ordinary input decoding");
            const auto own = test::prepareOwnEllipseForTest(testAsset.json);
            std::size_t caseComparisons = 0;
            for (const auto [width, height] : viewports) {
                test::NativeEllipseOracle oracle(testAsset.json);
                const auto bound = runtime::detail::bindNativeEllipseModel(
                    *decoded.input, *oracle.model());
                require(bool(bound), testAsset.name + " independent ordinary model binding");
                require(oracle.model().get() != own->model.get(),
                    testAsset.name + " independent original model ownership");
                auto stream = render::detail::OwnNativeEllipseStream::create(own);
                require(bool(stream), testAsset.name + " own stream creation");
                SequenceContext context{oracle, *stream.stream, *bound.binding,
                    own->authored->binding};
                context.sourceHash = test::ownRawHash(testAsset.json);
                for (std::size_t frame = 0; frame <= 60; ++frame)
                    compareFrame(context, testAsset.name, "forward", frame, width, height);
                for (std::size_t frame = 61; frame-- > 0;)
                    compareFrame(context, testAsset.name, "reverse", frame, width, height);
                for (const auto frame : seeks)
                    compareFrame(context, testAsset.name, "seeks", frame, width, height);
                require(context.comparisons == 130 && context.ordinal == 130,
                    testAsset.name + " viewport comparison count");
                caseComparisons += context.comparisons;
                directSamples += oracle.directSampleCount();
                directParses += oracle.directParseCount();
            }
            test::NativeEllipseOracle oracle(testAsset.json);
            const auto bound = runtime::detail::bindNativeEllipseModel(
                *decoded.input, *oracle.model());
            require(bool(bound), testAsset.name + " mixed ordinary binding");
            auto stream = render::detail::OwnNativeEllipseStream::create(own);
            require(bool(stream), testAsset.name + " mixed own stream");
            SequenceContext mixed{oracle, *stream.stream, *bound.binding,
                own->authored->binding};
            mixed.sourceHash = test::ownRawHash(testAsset.json);
            for (std::size_t index = 0; index < seeks.size(); ++index) {
                const auto [width, height] = viewports[index % viewports.size()];
                compareFrame(mixed, testAsset.name, "mixed", seeks[index], width, height);
            }
            require(mixed.comparisons == 8 && mixed.ordinal == 8,
                testAsset.name + " mixed comparison count");
            caseComparisons += mixed.comparisons;
            directSamples += oracle.directSampleCount();
            directParses += oracle.directParseCount();
            require(caseComparisons == 528, testAsset.name + " case comparison count");
            comparisons += caseComparisons;
            std::cout << "CASE " << testAsset.name << " planned=528 executed="
                      << caseComparisons << '\n';
        }
        require(comparisons == 7920, "full matrix comparison count");
        std::cout << "MATRIX planned=7920 executed=" << comparisons
                  << " ordinaryDirectSamples=" << directSamples
                  << " oracleDirectParses=" << directParses
                  << " mutationWitnesses=" << witnesses << '\n';
        std::cout << "own stream ordinary differential passed\n";
    } catch (const std::exception& error) {
        std::cerr << "FAILED: " << error.what() << '\n';
        return 1;
    }
}
