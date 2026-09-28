#include "OwnNativeEllipsePlayback.hpp"
#include "OwnNativeEllipseStream.hpp"
#include "avemotion/render/RenderPlanner.hpp"
#include "support/OwnVectorClippingTestData.hpp"
#include "support/OwnVectorTestData.hpp"
#include <iostream>
#include <limits>
#include <set>
#include "support/OwnVectorInstanceTestData.hpp"
using namespace avemotion;
using namespace avemotion::test;
void realRectangularProbes(render::detail::OwnNativeEllipseStream &stream) {
    for (const auto [width, height] : {std::pair{512U, 256U}, {256U, 512U}}) {
        const auto first = stream.emit(0, 256, 256);
        vectorRequire(bool(first), first.message);
        const auto probe = stream.emit(0, width, height);
        std::cout << "real direct rectangle " << width << 'x' << height
                  << " emitted=" << bool(probe) << " diagnostic=" << probe.message << '\n';
        if (width > height)
            vectorRequire(bool(probe), "known landscape frame0 remains provably supported");
        else
            vectorRequire(!probe && !probe.scene &&
                              probe.code ==
                                  render::detail::OwnNativeEllipseFrameCode::UnsupportedClipping,
                          "known portrait frame0 conservatively rejects required clip proof");
        const auto recovery = stream.emit(0, 256, 256);
        vectorRequire(bool(recovery) &&
                          recovery.scene->fingerprints.scene == first.scene->fingerprints.scene,
                      "real rectangle probe recovers square output");
        if (!probe)
            vectorRequire(!recovery.scene->changes.visualChanged &&
                              !recovery.scene->changes.firstEvaluation,
                          "real rejected rectangle retains successful history");
    }
}
void clippingEquivalence() {
    unsigned failures = 0;
    const auto check = [&](const std::string &json, std::size_t width, std::size_t height,
                           bool supported, const std::string &label) {
        const auto compiled = vectorCompile(json);
        vectorRequire(bool(compiled), label + compiled.path + compiled.message);
        const auto prepared = render::detail::prepareOwnVectorAsset(compiled.prepared);
        vectorRequire(bool(prepared), label + prepared.message);
        auto stream = render::detail::OwnNativeEllipseStream::create(prepared.prepared);
        const auto first = stream.stream->emit(0, 128, 128);
        vectorRequire(bool(first), "square target clipping equivalent: " + label);
        const auto rectangle = stream.stream->emit(0, width, height);
        if (bool(rectangle) != supported ||
            (!supported &&
             (rectangle.scene ||
              rectangle.code != render::detail::OwnNativeEllipseFrameCode::UnsupportedClipping ||
              rectangle.message.empty()))) {
            std::cerr << "clip contract mismatch: " << label << " emitted=" << bool(rectangle)
                      << '\n';
            ++failures;
        }
        const auto recovery = stream.stream->emit(0, 128, 128);
        vectorRequire(bool(recovery) && recovery.scene->evaluationSequence == 3,
                      "clipping recovery sequence: " + label);
        if (!supported &&
            (recovery.scene->changes.firstEvaluation || recovery.scene->changes.visualChanged ||
             recovery.scene->fingerprints.scene != first.scene->fingerprints.scene)) {
            std::cerr << "failed clip attempt changed history: " << label << '\n';
            ++failures;
        }
    };
    check(clippingPrecomp(clippingShape(-8, 20, 40, 100)), 256, 128, false, "wide fill crossing");
    check(clippingPrecomp(clippingShape(20, -8, 100, 40)), 128, 256, false, "tall fill crossing");
    for (int cap : {1, 2, 3})
        for (int join : {1, 2, 3})
            check(clippingPrecomp(vectorReplace(clippingShape(2, 30, 60, 90, true, cap, join),
                                                "\"c\":true", "\"c\":false")),
                  256, 128, false,
                  "stroke-only overflow cap/join=" + std::to_string(cap) + "/" +
                      std::to_string(join));
    // This central stroke leaves ample room for both pen and raster bounds.
    const auto contained = clippingPrecomp(clippingShape(56, 56, 72, 72, true));
    check(contained, 256, 128, true, "wide contained stroke");
    check(contained, 128, 256, true, "tall contained stroke");
    check(clippingPrecomp(clippingShape(40, 40, 80, 80, true)), 256, 128, true,
          "radius-relative miter bound contained");
    check(clippingPrecomp(clippingShape(40, -20, 80, 140)), 256, 128, true,
          "wide overflow only on target-coincident top/bottom");
    check(clippingPrecomp(clippingShape(-20, 40, 140, 80)), 128, 256, true,
          "tall overflow only on target-coincident left/right");
    check(clippingPrecomp(clippingShape(-8, 20, 40, 100)), 129, 128, false,
          "half-pixel inset is not target-coincident");
    check(contained, std::numeric_limits<std::size_t>::max(), 128, false,
          "unrepresentable narrow canvas at huge viewport offset");
    auto acute = clippingShape(20, 60, 64, 68, true, 1, 2);
    acute = vectorReplace(acute, "[[20,60],[64,60],[64,68],[20,68]]",
                          "[[20,64],[64,60],[64,68],[20,64]]");
    check(clippingPrecomp(acute), 256, 128, true, "round acute join stays contained");
    acute = vectorReplace(acute, "\"lj\":2,\"ml\":4", "\"lj\":1,\"ml\":16");
    check(clippingPrecomp(acute), 256, 128, false, "acute high-miter footprint crosses edge");
    const auto outsideRoot =
        vectorReplace(clippingShape(-20, -20, 140, 140), "\"ind\":1", "\"ind\":2");
    check(clippingPrecomp(clippingShape(40, 40, 80, 80), outsideRoot), 256, 128, true,
          "unrelated root draw excluded");
    vectorRequire(failures == 0, "clipping contract failures=" + std::to_string(failures));
}
void lifecycle(const std::string &json) {
    auto parsed = formats::detail::readOwnJson(json, {65536, 32});
    vectorRequire(bool(parsed), "motion read");
    auto prepared = render::detail::prepareOwnMotionAsset(*parsed.document);
    vectorRequire(bool(prepared),
                  "structural motion preparation: " + prepared.path + prepared.message);
    vectorRequire(!prepared.prepared->authored && prepared.prepared->vectorAuthored &&
                      prepared.prepared->program,
                  "one vector owner alternative");
    std::set<std::uint64_t> identities;
    for (auto count : {1U, 4U, 16U}) {
        std::vector<std::unique_ptr<render::detail::OwnNativeEllipseStream>> streams;
        for (unsigned i = 0; i < count; ++i) {
            auto created = render::detail::OwnNativeEllipseStream::create(prepared.prepared);
            vectorRequire(bool(created), created.message);
            streams.push_back(std::move(created.stream));
        }
        for (auto &stream : streams) {
            const auto first = stream->emit(179, 128, 128);
            vectorRequire(bool(first), first.message);
            vectorRequire(identities.insert(first.scene->instanceId).second,
                          "unique stream identity");
            vectorRequire(first.scene->changes.firstEvaluation &&
                              first.scene->evaluationSequence == 1,
                          "independent stream history");
            const auto failed = stream->emit(0, 0, 128);
            vectorRequire(!failed && !failed.scene &&
                              failed.code ==
                                  render::detail::OwnNativeEllipseFrameCode::InvalidViewport,
                          "invalid viewport publishes no scene");
            const auto same = stream->emit(179, 128, 128);
            vectorRequire(bool(same) && same.scene->evaluationSequence == 3 &&
                              !same.scene->changes.firstEvaluation &&
                              !same.scene->changes.visualChanged,
                          "failed attempt preserves successful fingerprints");
            const auto big = stream->emit(179, 512, 512);
            vectorRequire(bool(big), big.message);
            const auto restored = stream->emit(179, 128, 128);
            vectorRequire(bool(restored) &&
                              restored.scene->fingerprints.scene == first.scene->fingerprints.scene,
                          "viewport round trip reproduces frame");
            for (auto f : {0U, 15U, 90U, 15U, 179U, 0U})
                vectorRequire(bool(stream->emit(f, 128, 128)), "repeat seek");
            for (const auto &draw : first.scene->drawItems) {
                vectorRequire(draw.sourceGeometryId ==
                                      static_cast<std::uint64_t>(draw.modelGeometry.value) + 1 &&
                                  draw.sourcePaintId ==
                                      static_cast<std::uint64_t>(draw.modelPaint.value) + 1,
                              "canonical source identity modelID+1");
            }
        }
    }
    auto playback = render::detail::OwnNativeEllipsePlayback::create(prepared.prepared);
    vectorRequire(bool(playback), "vector playback");
    vectorRequire(playback.playback->frameAtPosition(1) == 179 &&
                      playback.playback->frameAtPosition(0) == 0,
                  "vector frame mapping endpoints");
    vectorRequire(playback.playback->durationSeconds() == static_cast<double>(179.0F / 60.0F),
                  "float-rounded duration retained");
    auto created = render::detail::OwnNativeEllipseStream::create(prepared.prepared);
    auto retained = created.stream->emit(0, 128, 128);
    std::weak_ptr<const model::MotionAssetModel> weak = prepared.prepared->model;
    created.stream.reset();
    playback.playback.reset();
    prepared.prepared.reset();
    vectorRequire(!weak.expired() && retained.scene->assetModel,
                  "scene retains model after owner retirement");
    retained.scene.reset();
    vectorRequire(weak.expired(), "model retires with last scene");
}
void visibilityAndOrder() {
    auto top = vectorShapeLayer("{\"k\":" + vectorShape(3, true) + "}");
    top = vectorReplace(top, "{\"ty\":\"tr\"}",
                        "{\"ty\":\"fl\",\"c\":{\"k\":[1,0,0,1]}},{\"ty\":\"tr\"}");
    auto bottom = vectorReplace(top, "\"ind\":1", "\"ind\":2");
    bottom = vectorReplace(bottom, "\"ip\":0,\"op\":180", "\"ip\":10,\"op\":20");
    bottom = vectorReplace(bottom, "[1,0,0,1]", "[0,1,0,1]");
    const auto parsed = formats::detail::readOwnJson(vectorRoot(top + "," + bottom), {65536, 32});
    auto prepared = render::detail::prepareOwnMotionAsset(*parsed.document);
    vectorRequire(bool(prepared), prepared.message);
    auto stream = render::detail::OwnNativeEllipseStream::create(prepared.prepared);
    for (auto f : {0U, 10U, 19U, 20U, 10U}) {
        auto result = stream.stream->emit(f, 128, 128);
        vectorRequire(bool(result), result.message);
        const bool both = f >= 10 && f < 20;
        vectorRequire(result.scene->drawItems.size() == (both ? 2U : 1U),
                      "half-open visibility/reactivation");
        for (std::size_t i = 0; i < result.scene->drawItems.size(); ++i)
            vectorRequire(result.scene->drawItems[i].drawOrder == i,
                          "visible draw ordinal after hidden layer");
        vectorRequire(result.scene->drawItems.back().paint.solid.r == 255,
                      "front authored layer is last draw");
        for (const auto &l : result.scene->layers)
            for (std::uint32_t i = 0; i < l.drawItemCount; ++i)
                vectorRequire(result.scene->drawItems.at(l.firstDrawItem + i).layerIndex ==
                                  l.modelLayer.index(),
                              "actual per-layer draw range");
    }
}
void staticLocalDependencies(const std::string &fixture) {
    const auto moving = vectorReplace(fixture, "\"p\":{\"a\":0,\"k\":[30,40]}",
                                      "\"p\":{\"a\":1,\"k\":[{\"t\":0,\"s\":[30,"
                                      "40],\"h\":1},{\"t\":10,\"s\":[50,60]}]}");
    const auto check = [&](const std::string &json, bool trimAnimated) {
        const auto parsed = formats::detail::readOwnJson(json, {65536, 32});
        const auto prepared = render::detail::prepareOwnMotionAsset(*parsed.document);
        vectorRequire(bool(prepared), prepared.message);
        auto stream = render::detail::OwnNativeEllipseStream::create(prepared.prepared);
        const auto a = stream.stream->emit(0, 128, 128), b = stream.stream->emit(10, 128, 128);
        vectorRequire(bool(a) && bool(b), "moving parent scenes");
        const auto &first = a.scene->drawItems[0];
        const auto &next = b.scene->drawItems[0];
        vectorRequire(first.localToViewport.dx != next.localToViewport.dx,
                      "parent movement separated");
        if (!trimAnimated) {
            vectorRequire(first.canonicalGeometry &&
                              first.canonicalGeometry == next.canonicalGeometry &&
                              first.localPath.hash == next.localPath.hash,
                          "moving parent preserves static local path cache");
        } else {
            vectorRequire(!first.canonicalGeometry && !next.canonicalGeometry &&
                              first.localPath.hash != next.localPath.hash,
                          "changing trim invalidates local-static classification");
        }
    };
    check(moving, false);
    check(vectorReplace(moving, "\"e\":{\"a\":0,\"k\":95}",
                        "\"e\":{\"a\":1,\"k\":[{\"t\":0,\"s\":[95],\"h\":1},{"
                        "\"t\":10,\"s\":[50]}]}"),
          true);
}
int main(int argc, char **argv) {
    try {
        const auto json = vectorInput(argc, argv);
        lifecycle(json);
        auto compiled = vectorCompile(json);
        vectorRequire(bool(compiled), "vector compile: " + compiled.path + compiled.message);
        auto prepared = render::detail::prepareOwnVectorAsset(compiled.prepared);
        vectorRequire(bool(prepared), "vector preparation: " + prepared.path + prepared.message);
        auto stream = render::detail::OwnNativeEllipseStream::create(prepared.prepared);
        vectorRequire(bool(stream), "stream creation: " + stream.message);
        if (argc != 1)
            realRectangularProbes(*stream.stream);
        auto scene = stream.stream->emit(0, 128, 128);
        vectorRequire(bool(scene), "vector emission: " + scene.message);
        vectorRequire(scene.scene->drawItems.size() == (argc == 1 ? 2U : 36U),
                      "all painted vector draws emitted");
        if (argc == 1) {
            const auto &fill = scene.scene->drawItems[0];
            const auto &stroke = scene.scene->drawItems[1];
            const auto &strokeGeometry =
                prepared.prepared->model->geometries[stroke.modelGeometry.index()];
            const auto &strokePaint = prepared.prepared->model->paints[stroke.modelPaint.index()];
            vectorRequire(strokeGeometry.resourceClass == model::ResourceClass::InstanceEvaluated &&
                              !strokeGeometry.staticValue &&
                              strokePaint.resourceClass ==
                                  model::ResourceClass::InstanceEvaluated &&
                              !strokePaint.staticValue,
                          "transformed final-space stroke must not expose local "
                          "static resources to backend");
            vectorRequire(!fill.stroke.enabled && stroke.stroke.enabled, "fill before stroke");
            vectorRequire(stroke.localPath.points.back().x < 10,
                          "outer trim changes stroke endpoint");
            vectorRequire(stroke.separatedOpacity == 1,
                          "zero-opacity transform parent keeps child visible");
        }
        std::cout << "vector stream emitted " << scene.scene->drawItems.size() << " draws\n";
        if (argc == 1) {
            instanceContracts();
            clippingEquivalence();
            const auto legacy = vectorRead(std::filesystem::path(AVEMOTION_FIXTURE_DIR) /
                                           "telegram_sticker_basic.json");
            const auto legacyJson = formats::detail::readOwnJson(legacy);
            const auto legacyPrepared = render::detail::prepareOwnMotionAsset(*legacyJson.document);
            vectorRequire(bool(legacyPrepared) && legacyPrepared.prepared->authored &&
                              !legacyPrepared.prepared->vectorAuthored,
                          "structural dispatcher keeps primitive owner alternative");
            visibilityAndOrder();
            staticLocalDependencies(json);
            const auto animated = vectorRead(std::filesystem::path(AVEMOTION_FIXTURE_DIR) /
                                             "own_vector/animated.json");
            lifecycle(animated);
            auto parsed = formats::detail::readOwnJson(vectorReplace(json, "\"fr\":60", "\"fr\":0"),
                                                       {65536, 32});
            const auto rejected = render::detail::prepareOwnMotionAsset(*parsed.document);
            vectorRequire(!rejected && !rejected.prepared && rejected.path == "/fr" &&
                              !rejected.message.empty(),
                          "failed vector admission returns actionable diagnostic atomically");
        }
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
