#include "OwnNativeEllipsePlayback.hpp"
#include "support/OwnVectorGradientTestData.hpp"
#include "support/OwnVectorMatteTestData.hpp"
#include "../src/model/AssetModelBuilder.hpp"
#include "avemotion/runtime/RecordingBackend.hpp"
#include "OwnNativeEllipseStream.hpp"
#include "avemotion/render/RenderPlanner.hpp"
#include "support/OwnVectorClippingTestData.hpp"
#include "support/OwnVectorTestData.hpp"
#include <iostream>
#include <limits>
#include <set>
#include <tuple>
#include "support/OwnVectorInstanceTestData.hpp"
#include "support/OwnVectorGroupTestData.hpp"
#include "support/OwnVectorMetadataPositionTestData.hpp"
using namespace avemotion;
using namespace avemotion::test;
void metadataPositionStream() {
    // Catch ignored split axes, shared axis knots, lost parent transforms, stale
    // reverse sampling, occurrence clock leakage, and annotation side effects.
    for (bool occurrences : {false,true}) {
        auto stream = instanceStream(metadataPositionFixture(true,occurrences));
        auto plain = instanceStream(metadataPositionFixture(false,occurrences));
        const std::vector<std::tuple<unsigned,std::vector<model::MotionVec2Value>>> samples = occurrences
            ? std::vector<std::tuple<unsigned,std::vector<model::MotionVec2Value>>>{
                {0,{{3,4}}}, {14,{{5,4},{10,4}}}, {15,{{5.5F,4},{10.5F,14}}},
                {20,{{8,4},{13,14}}}, {25,{{10.5F,14},{15.5F,14}}}, {40,{{18,14},{23,14}}},
                {60,{{23,14},{23,-1}}}, {70,{{23,-1},{23,-1}}}, {179,{{23,-1},{23,-1}}},
                {20,{{8,4},{13,14}}}, {0,{{3,4}}}}
            : std::vector<std::tuple<unsigned,std::vector<model::MotionVec2Value>>>{
                {0,{{3,4}}}, {14,{{10,4}}}, {15,{{10.5F,14}}}, {20,{{13,14}}},
                {40,{{23,14}}}, {59,{{23,14}}}, {60,{{23,-1}}}, {179,{{23,-1}}},
                {15,{{10.5F,14}}}, {0,{{3,4}}}};
        for (const auto& [frame,expected] : samples) {
            const auto actual = stream->emit(frame,128,128), unannotated = plain->emit(frame,128,128);
            vectorRequire(bool(actual) && bool(unannotated),"metadata/position emit");
            vectorRequire(actual.scene->drawItems.size() == expected.size() &&
                          actual.scene->fingerprints.scene == unannotated.scene->fingerprints.scene,
                          "annotations leave sampled scene content unchanged occurrences=" + std::to_string(occurrences) +
                          " frame=" + std::to_string(frame) + " draws=" + std::to_string(actual.scene->drawItems.size()) +
                          " expected=" + std::to_string(expected.size()) + " topology=" +
                          std::to_string(actual.scene->fingerprints.topology) + "/" + std::to_string(unannotated.scene->fingerprints.topology) +
                          " geometry=" + std::to_string(actual.scene->fingerprints.geometry) + "/" + std::to_string(unannotated.scene->fingerprints.geometry) +
                          " paint=" + std::to_string(actual.scene->fingerprints.paint) + "/" + std::to_string(unannotated.scene->fingerprints.paint));
            std::set<std::uint32_t> identities;
            for(std::size_t i=0;i<expected.size();++i) {
                const auto& draw = actual.scene->drawItems[i];
                const auto& other = unannotated.scene->drawItems[i];
                vectorRequire(std::abs(draw.localToViewport.dx - expected[i].x) < 0.0001F &&
                              std::abs(draw.localToViewport.dy - expected[i].y) < 0.0001F &&
                              draw.localToViewport.m11 == 1 && draw.localToViewport.m22 == 1 &&
                              draw.separatedOpacity == 1 && draw.drawOrder == i,
                              "literal split/parent/occurrence matrix frame=" + std::to_string(frame));
                vectorRequire(draw.modelNode == other.modelNode && draw.modelGeometry == other.modelGeometry &&
                              draw.modelPaint == other.modelPaint && draw.sourcePathNode == other.sourcePathNode &&
                              draw.sourcePaintNode == other.sourcePaintNode && draw.localPath.verbs == other.localPath.verbs &&
                              std::equal(draw.localPath.points.begin(),draw.localPath.points.end(),
                                         other.localPath.points.begin(),other.localPath.points.end(),
                                         [](const auto& a,const auto& b) { return a.x == b.x && a.y == b.y; }) &&
                              draw.paint.solid.r == other.paint.solid.r && draw.paint.solid.g == other.paint.solid.g &&
                              draw.paint.solid.b == other.paint.solid.b && draw.paint.solid.a == other.paint.solid.a &&
                              identities.insert(draw.modelNode.value).second,
                              "metadata preserves geometry, paint, source IDs and occurrence identities");
            }
        }
    }
    std::cout << "PASS metadata/position stream: independent axes, parents, repeated clocks and annotation invariance\n";
}
void boundedGroupStream() {
    auto stream = instanceStream(vectorGroupFixture());
    std::set<std::uint32_t> identities;
    for (const auto [frame, heldAlpha, linearAlpha] : {std::tuple{0U, 0U, 127U},
                                                       {50U, 0U, 63U},
                                                       {68U, 0U, 40U},
                                                       {69U, 63U, 39U},
                                                       {100U, 63U, 0U},
                                                       {110U, 127U, 0U},
                                                       {68U, 0U, 40U},
                                                       {69U, 63U, 39U}}) {
        const auto scene = stream->emit(frame, 128, 128);
        vectorRequire(bool(scene) && scene.scene->drawItems.size() == 10,
                      "ten group draws including zero alpha");
        identities.clear();
        for (unsigned d = 0; d < 10; ++d) {
            const auto &draw = scene.scene->drawItems[d];
            identities.insert(draw.modelNode.value);
            const unsigned group = 9 - d;
            vectorRequire(draw.drawOrder == d && draw.paint.solid.a == (group == 9  ? 127U
                                                                        : group % 2 ? heldAlpha
                                                                                    : linearAlpha),
                          "literal reverse group order and inherited alpha frame=" +
                              std::to_string(frame));
        }
        vectorRequire(identities.size() == 10, "unique sibling draw identities");
    }
    auto second = instanceStream(vectorGroupFixture());
    const auto independent = second->emit(69, 256, 128);
    vectorRequire(bool(independent) && independent.scene->evaluationSequence == 1 &&
                      independent.scene->drawItems.size() == 10 &&
                      independent.scene->drawItems[0].paint.solid.a == 127,
                  "independent group stream and rectangular viewport");
    for (const auto &opacity : {std::string(R"({"k":0})"), std::string(R"({"k":50})"),
                                std::string(R"({"k":100})"), std::string(R"({"k":0.00005})")}) {
        auto outer = instanceStream(vectorOuterGroupFixture(opacity));
        const auto scene = outer->emit(0, 128, 128);
        vectorRequire(bool(scene) && scene.scene->drawItems.size() == 2 &&
                          scene.scene->drawItems[0].stroke.enabled &&
                          scene.scene->drawItems[0].paint.solid.a == 127,
                      "outer stroke retains layer alpha when path group fades");
    }
}
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
std::string measuredShapeLayer(const std::string &items) {
    return "{\"ty\":4,\"ind\":1,\"ip\":0,\"op\":180,\"ks\":{},\"shapes\":[" +
           items + "]}";
}
std::string measuredGroup(const std::string &middle, const std::string &transform = "") {
    return "{\"ty\":\"gr\",\"it\":[{\"ty\":\"sh\",\"ks\":{\"a\":0,\"k\":" +
           vectorShape(2, false) + "}}," + middle + ",{\"ty\":\"tr\"" + transform + "}]}";
}
void measuredShapeScopeAndWidth() {
    const std::string fill = R"({"ty":"fl","c":{"a":0,"k":[1,0,0,1]}})";
    const std::string stroke = R"({"ty":"st","c":{"a":0,"k":[0,0,1,1]},"w":{"a":0,"k":2}})";
    const std::string half = R"({"ty":"tm","m":1,"s":{"a":0,"k":0},"e":{"a":0,"k":50}})";
    const auto emit = [](const std::string &json, unsigned frame = 0) {
        auto compiled = vectorCompile(json);
        vectorRequire(bool(compiled), "measured grammar compiles: " + compiled.path + " " + compiled.message);
        auto prepared = render::detail::prepareOwnVectorAsset(compiled.prepared);
        vectorRequire(bool(prepared), "measured grammar prepares: " + prepared.path + " " + prepared.message);
        auto stream = render::detail::OwnNativeEllipseStream::create(prepared.prepared);
        vectorRequire(bool(stream), "measured stream created");
        auto scene = stream.stream->emit(frame, 128, 128);
        vectorRequire(bool(scene), "measured grammar emits: " + scene.message);
        return *scene.scene;
    };
    const std::string animatedStroke = R"({"ty":"st","c":{"a":0,"k":[0,0,1,1]},"w":{"a":1,"k":[{"t":0,"s":[0],"h":1},{"t":10,"s":[10],"h":1},{"t":20,"s":[0]}]}})";
    const auto widthJson = vectorRoot(measuredShapeLayer(measuredGroup(animatedStroke)));
    for (const auto &[frame, width] : {std::pair{0U, 0.F}, {10U, 10.F}, {20U, 0.F}}) {
        const auto scene = emit(widthJson, frame);
        vectorRequire(scene.drawItems.size() == 1 && scene.drawItems[0].stroke.enabled &&
                          std::abs(scene.drawItems[0].stroke.width - width) < 0.001F,
                      "animated width keeps enabled stroke including zero");
    }
    const auto nestedWidth = vectorReplace(
        vectorPrecomp(measuredShapeLayer(measuredGroup(animatedStroke))),
        "\"ip\":10,\"op\":170,\"ks\":{}",
        "\"ip\":10,\"op\":170,\"st\":10,\"ks\":{}");
    const auto nestedAt10 = emit(nestedWidth, 10).drawItems[0].stroke.width;
    const auto nestedAt20 = emit(nestedWidth, 20).drawItems[0].stroke.width;
    vectorRequire(nestedAt10 == 0.F && std::abs(nestedAt20 - 10.F) < 0.001F,
                  "nested animated width samples containing local clock during preparation and stream: " +
                      std::to_string(nestedAt10) + "/" + std::to_string(nestedAt20));
    const auto negativeEaseStroke = vectorReplace(animatedStroke,
        R"({"t":0,"s":[0],"h":1})",
        R"({"t":0,"s":[0],"i":{"x":0.75,"y":-2},"o":{"x":0.25,"y":-2}})");
    const auto negativeJson = vectorRoot(measuredShapeLayer(measuredGroup(negativeEaseStroke)));
    const auto negativeModel = vectorCompile(negativeJson);
    vectorRequire(bool(negativeModel), "bounded negative ease admitted");
    const auto negativePrepared = render::detail::prepareOwnVectorAsset(negativeModel.prepared);
    vectorRequire(bool(negativePrepared), "negative ease initial width0 prepares");
    auto negativeStream = render::detail::OwnNativeEllipseStream::create(negativePrepared.prepared);
    const auto negativeFrame = negativeStream.stream->emit(5, 128, 128);
    vectorRequire(!negativeFrame && !negativeFrame.scene && !negativeFrame.message.empty(),
                  "negative evaluated width rejects without publishing a scene");
    for (const auto &middle : {half + "," + stroke, stroke + "," + half}) {
        const auto scene = emit(vectorRoot(measuredShapeLayer(measuredGroup(
            middle, R"(,"p":{"a":0,"k":[20,20]},"s":{"a":0,"k":[75.153,75.153]})"))));
        vectorRequire(scene.drawItems.size() == 1 && scene.drawItems[0].stroke.enabled,
                      "inline trim retains painted stroke");
        const auto &draw = scene.drawItems[0];
        vectorRequire(std::abs(draw.localPath.points.back().x - 5.F) < 0.01F &&
                          std::abs(draw.path.points.back().x - 23.75765F) < 0.02F,
                      "inline trim measures raw path before group scale");
    }
    const auto shared = emit(vectorRoot(measuredShapeLayer(
        measuredGroup(stroke, R"(,"p":{"a":0,"k":[20,20]})") + "," +
        measuredGroup(stroke, R"(,"p":{"a":0,"k":[40,20]})") + "," + half)));
    vectorRequire(shared.drawItems.size() == 2 &&
                      std::abs(shared.drawItems[0].path.points.back().x - 45.F) < 0.02F &&
                      std::abs(shared.drawItems[1].path.points.back().x - 25.F) < 0.02F,
                  "trailing simultaneous trim independently reaches both sibling paths");
    const auto outer = emit(vectorRoot(measuredShapeLayer(measuredGroup(
        fill, R"(,"p":{"a":0,"k":[20,20]},"s":{"a":0,"k":[75.153,75.153]})") +
        "," + stroke)));
    vectorRequire(outer.drawItems.size() == 2 && outer.drawItems[0].stroke.enabled &&
                      !outer.drawItems[1].stroke.enabled &&
                      std::abs(outer.drawItems[0].stroke.width - 2.F) < 0.001F &&
                      std::abs(outer.drawItems[0].path.points.back().x - 27.5153F) < 0.02F,
                  "outer stroke paints first in layer width over scaled group path");
}
void gradientStream() {
    const auto compiled = vectorCompile(gradientParentFixture());
    vectorRequire(bool(compiled), "gradient stream admission: " + compiled.path + " " + compiled.message);
    const auto prepared = render::detail::prepareOwnVectorAsset(compiled.prepared);
    vectorRequire(bool(prepared), "gradient prepares: " + prepared.path + " " + prepared.message);
    auto stream = render::detail::OwnNativeEllipseStream::create(prepared.prepared);
    vectorRequire(bool(stream), "gradient stream created");
    const auto first = stream.stream->emit(0, 128, 128);
    vectorRequire(bool(first) && first.scene->drawItems.size() == 1, "transform-parent opacity0 preserves child gradient");
    const auto &d = first.scene->drawItems[0];
    vectorRequire(d.paint.kind == runtime::PaintKind::Gradient && d.paint.gradient.kind == runtime::GradientKind::Linear &&
                      d.paint.gradient.start.x == 15 && d.paint.gradient.start.y == 11.25F &&
                      d.paint.gradient.end.x == 63 && d.paint.gradient.end.y == 11.25F, "literal final gradient endpoints");
    vectorRequire(d.localPaintAvailable && d.localPaintStaticCandidate && d.opacitySeparated && d.separatedOpacity == 1 &&
                      d.localPaint.gradient.start.x == 2 && d.localPaint.gradient.start.y == 3 &&
                      d.localPaint.gradient.end.x == 18 && d.localPaint.gradient.end.y == 3 &&
                      d.localToViewport.m11 == 3 && d.localToViewport.m22 == 0.75F &&
                      d.localToViewport.dx == 9 && d.localToViewport.dy == 9, "canonical local paint and full affine separated");
    vectorRequire(d.localPaint.gradient.stops.size() == 3 && d.localPaint.gradient.stops[1].position == 0.5F &&
                      d.localPaint.gradient.stops[1].color.r == 127 && d.localPaint.gradient.stops[1].color.g == 63 &&
                      d.localPaint.gradient.stops[1].color.b == 31 && d.localPaint.gradient.stops[1].color.a == 255,
                  "literal middle RGB byte truncation");
    vectorRequire(d.paint.solid.a == 0 && d.paint.solid.r == 0 && !d.paint.image.present &&
                      d.paint.gradient.center.x == 0 && d.paint.gradient.focal.y == 0 &&
                      d.paint.gradient.centerRadius == 0 && d.paint.gradient.focalRadius == 0,
                  "neutral inactive gradient fields");
    vectorRequire(first.scene->statistics.gradientPaintCount == 1 && first.scene->statistics.gradientStopCount == 3 &&
                      first.scene->statistics.solidPaintCount == 0, "accurate gradient scene counts");
    for (const auto f : {179U, 10U, 0U, 10U}) {
        const auto scene = stream.stream->emit(f, 256, 256);
        vectorRequire(bool(scene), "gradient repeated seek");
        const auto &next = scene.scene->drawItems[0];
        vectorRequire(next.canonicalPaint == d.canonicalPaint && next.localPaint.gradient.start.x == 2 &&
                          next.paint.gradient.start.x == 30 && next.paint.gradient.end.x == 126,
                      "presentation preserves canonical static paint identity");
    }
    // Reference composes float fill alpha with inherited float alpha before truncating once.
    struct AlphaCase { const char *fill, *group, *layer; unsigned local, final; };
    for (const auto &c : {AlphaCase{"50.5","50.5","50.5",128,32}, {"33.3","66.7","99.9",84,56},
                          {"99.9","99.9","99.9",254,254}, {"1.1","75.3","80.2",2,1}}) {
        const auto fill = vectorReplace(gradientFill(), "\"k\":100", "\"k\":" + std::string(c.fill));
        auto m = vectorCompile(gradientFixture(fill, ",\"o\":{\"k\":" + std::string(c.group) + "}",
                                                 "\"o\":{\"k\":" + std::string(c.layer) + "}"));
        vectorRequire(bool(m), "fractional gradient admission");
        const auto p = render::detail::prepareOwnVectorAsset(m.prepared);
        vectorRequire(bool(p), "fractional gradient preparation");
        auto s = render::detail::OwnNativeEllipseStream::create(p.prepared);
        const auto v = s.stream->emit(0,128,128);
        vectorRequire(bool(v) && v.scene->drawItems[0].localPaint.gradient.stops[0].color.a == c.local &&
                          v.scene->drawItems[0].paint.gradient.stops[0].color.a == c.final,
                      "pinned fractional fill/group/layer alpha=" + std::to_string(c.final));
    }
    auto moving = instanceStream(gradientAnimatedFixture());
    std::shared_ptr<const runtime::CanonicalPaint> canonical;
    for (const auto &[f,x,y,point] : {std::tuple{0U,15.F,11.25F,0.F}, {10U,19.F,15.25F,2.F},
                                     {90U,15.F,11.25F,0.F}, {179U,15.F,11.25F,0.F},
                                     {10U,19.F,15.25F,2.F}, {0U,15.F,11.25F,0.F}}) {
        const auto scene = moving->emit(f,128,128);
        vectorRequire(bool(scene) && scene.scene->drawItems.size() == 1, "animated gradient parent/path emits");
        const auto &draw = scene.scene->drawItems[0];
        vectorRequire(draw.paint.gradient.start.x == x && draw.paint.gradient.start.y == y &&
                          draw.localPath.points[0].x == point && !draw.localGeometryStaticCandidate &&
                          draw.localPaintStaticCandidate && draw.localPaint.gradient.start.x == 2,
                      "literal animated parent/path forward/reverse/repeat frame=" + std::to_string(f));
        if (!canonical) canonical = draw.canonicalPaint;
        vectorRequire(canonical && draw.canonicalPaint == canonical,
                      "geometry animation preserves asset-static gradient paint");
    }
    auto occurrences = instanceStream(gradientOccurrencesFixture());
    for (const auto &[f,positions] : std::vector<std::pair<unsigned,std::vector<float>>>{
            {0,{22}}, {10,{22,32}}, {20,{32,32}}, {179,{32,32}}, {10,{22,32}}, {0,{22}}}) {
        const auto scene = occurrences->emit(f,128,128);
        vectorRequire(bool(scene), "gradient occurrence emits: " + scene.message);
        std::vector<float> actual;
        std::set<std::uint64_t> identities;
        for (const auto &draw : scene.scene->drawItems) {
            actual.push_back(draw.paint.gradient.start.x);
            identities.insert(draw.sourcePaintId);
            vectorRequire(draw.localPaint.gradient.start.x == 2 && draw.localPaintStaticCandidate,
                          "occurrence canonical paint stays local");
        }
        std::sort(actual.begin(),actual.end());
        vectorRequire(actual == positions && identities.size() == actual.size(),
                      "literal occurrence clocks/identity frame=" + std::to_string(f));
    }
    std::cout << "owned static gradient local/final RGB, parent opacity0, full affine, fractional alpha,"
                 " static paint identity, animated parent/path seeks and occurrence clocks verified\n";
}
void matteStream() {
    // Lost role indices, parenting-as-alpha, interval drift and double common
    // alpha all fail these independent literal observations.
    auto stream = instanceStream(matteFixture());
    for (const auto [width,height,contentX,contentY,providerX,providerY] : {
            std::tuple{256U,128U,79.F,27.F,81.F,31.F},
            std::tuple{128U,256U,15.F,91.F,17.F,95.F}}) {
        for (unsigned frame : {19U,20U,30U,31U,30U,20U,19U,20U}) {
            const auto emitted = stream->emit(frame,width,height);
            vectorRequire(bool(emitted), "matte scene emit");
            const auto& scene = *emitted.scene;
            vectorRequire(scene.layers.size()==4 && scene.layers[1].matteSourceLayer==2 &&
                scene.layers[2].matteTargetLayer==1 && scene.layers[1].matte==runtime::MatteMode::Alpha &&
                scene.layers[2].matte==runtime::MatteMode::None && scene.layers[1].parentLayer==0 &&
                scene.layers[2].parentLayer==0, "stable reciprocal scene roles independent of parenting");
            const bool active = frame==20 || frame==30;
            vectorRequire(scene.drawItems.size()==(active ? 2U:0U), "literal [20,31) interval");
            if (!active) continue;
            for (unsigned i=0;i<2;++i) {
                const auto& d=scene.drawItems[i];
                vectorRequire(d.layerIndex==i+1 && d.localToViewport.dx==(i?providerX:contentX) &&
                    d.localToViewport.dy==(i?providerY:contentY) && d.localToViewport.m11==1 &&
                    d.localToViewport.m22==1 && d.separatedOpacity==0.5F, "literal parent matrix and independent fractional alpha");
            }
            render::MotionRenderPlanner planner;
            render::PresentationState p; p.opacity=0.5F;
            const auto plan=planner.build(scene,p);
            vectorRequire(bool(plan) && plan.plan.drawItems.size()==2 && plan.plan.presentationOpacity==0.5F &&
                plan.plan.presentationVisible && plan.plan.drawItems[0].effectiveOpacity==0.25F &&
                plan.plan.drawItems[1].effectiveOpacity==0.5F && plan.plan.statistics.unsupportedFeatureItemCount==2 &&
                (plan.plan.drawItems[0].featureBits & render::RenderFeatureMatte) &&
                (plan.plan.drawItems[1].featureBits & render::RenderFeatureMatte), "public fail-closed pair and presentation alpha once");
        }
    }
    auto zeroJson=vectorReplace(matteFixture(),R"("p":{"k":[2,4]},"o":{"k":50})",R"("p":{"k":[2,4]},"o":{"k":0})");
    auto zero = instanceStream(zeroJson);
    const auto emptyProvider=zero->emit(20,128,128);
    vectorRequire(bool(emptyProvider) && emptyProvider.scene->drawItems.size()==1 &&
        emptyProvider.scene->layers[2].drawItemCount==0 && emptyProvider.scene->layers[2].matteTargetLayer==1 &&
        emptyProvider.scene->layers[1].matteSourceLayer==2, "empty provider keeps relationship identity");
    auto difference=instanceStream(vectorRoot(matteLayer(1,",\"bm\":10")));
    const auto d=difference->emit(0,128,128);
    vectorRequire(bool(d) && d.scene->layers[1].isolatedBottomDifference, "Difference preserved in scene");
    render::MotionRenderPlanner differencePlanner;
    const auto dp=differencePlanner.build(*d.scene);
    vectorRequire(bool(dp) && dp.plan.statistics.unsupportedFeatureItemCount==1 &&
        (dp.plan.drawItems[0].featureBits & render::RenderFeatureBlend), "isolated Difference unsupported publicly");
    auto twoPairs=instanceStream(vectorRoot(matteLayer(1,",\"td\":1")+","+matteLayer(2,",\"tt\":1")+","+
                                            matteLayer(3,",\"td\":1")+","+matteLayer(4,",\"tt\":1")));
    auto emitted=twoPairs->emit(0,128,128);
    vectorRequire(bool(emitted), "two pair topology witness emit");
    auto swapped=*emitted.scene;
    swapped.layers[1].matteSourceLayer=4; swapped.layers[4].matteTargetLayer=1;
    swapped.layers[3].matteSourceLayer=2; swapped.layers[2].matteTargetLayer=3;
    vectorRequire(runtime::computeSceneFingerprints(swapped).topology != emitted.scene->fingerprints.topology,
        "valid reciprocal relationship changes scene topology");
    render::MotionRenderPlanner topologyPlanner;
    const auto originalPlan=topologyPlanner.build(*emitted.scene), swappedPlan=topologyPlanner.build(swapped);
    vectorRequire(bool(originalPlan) && bool(swappedPlan) &&
        originalPlan.plan.fingerprints.topology!=swappedPlan.plan.fingerprints.topology &&
        originalPlan.plan.fingerprints.geometryIdentity==swappedPlan.plan.fingerprints.geometryIdentity &&
        originalPlan.plan.fingerprints.paintIdentity==swappedPlan.plan.fingerprints.paintIdentity,
        "valid reciprocal relationship changes plan topology only");
    auto originalModel=*emitted.scene->assetModel;
    auto changedModel=originalModel;
    unsigned first=0, second=0;
    for (auto& source:changedModel.sourceNodes) if(source.matteSource.valid()) {
        if (!first) first=source.id.value; else second=source.id.value;
    }
    std::swap(changedModel.sourceNodes[first].matteSource,changedModel.sourceNodes[second].matteSource);
    model::detail::refreshAssetModelDerivedData(changedModel);
    vectorRequire(originalModel.topologyFingerprint!=changedModel.topologyFingerprint &&
        originalModel.resourceFingerprint==changedModel.resourceFingerprint,
        "source relationships change immutable model topology without resource churn");
}
int main(int argc, char **argv) {
    try {
        if (argc==1) matteStream();
        const auto json = vectorInput(argc, argv);
        const bool second = argc == 3 && std::filesystem::path(argv[2]).filename() == "a-2.tgs";
        lifecycle(json);
        auto compiled = vectorCompile(json);
        vectorRequire(bool(compiled), "vector compile: " + compiled.path + compiled.message);
        auto prepared = render::detail::prepareOwnVectorAsset(compiled.prepared);
        vectorRequire(bool(prepared), "vector preparation: " + prepared.path + prepared.message);
        auto stream = render::detail::OwnNativeEllipseStream::create(prepared.prepared);
        vectorRequire(bool(stream), "stream creation: " + stream.message);
        if (argc != 1 && !second)
            realRectangularProbes(*stream.stream);
        auto scene = stream.stream->emit(0, 128, 128);
        vectorRequire(bool(scene), "vector emission: " + scene.message);
        vectorRequire(second ? !scene.scene->drawItems.empty() :
                      scene.scene->drawItems.size() == (argc == 1 ? 2U : 36U),
                      "all painted vector draws emitted");
        if (second) {
            for (unsigned target : {128U, 129U, 256U, 512U}) {
                for (unsigned f = 0; f < 180; ++f) {
                    const auto emitted = stream.stream->emit(f, target, target);
                    vectorRequire(bool(emitted), "a-2 forward frame=" + std::to_string(f) +
                                                   " target=" + std::to_string(target) +
                                                   " " + emitted.message);
                }
                for (unsigned f = 180; f-- > 0;) {
                    const auto emitted = stream.stream->emit(f, target, target);
                    vectorRequire(bool(emitted), "a-2 reverse frame=" + std::to_string(f) +
                                                   " target=" + std::to_string(target) +
                                                   " " + emitted.message);
                }
            }
            std::cout << "a-2 forward/reverse all 180 frames emitted at 128/129/256/512 square\n";
        }
        if (argc == 1) {
            gradientStream();
            measuredShapeScopeAndWidth();
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
            metadataPositionStream();
            instanceContracts();
            boundedGroupStream();
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
