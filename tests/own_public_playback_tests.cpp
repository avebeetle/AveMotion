#include "avemotion/render/OwnPlayback.hpp"
#include "avemotion/model/AssetModel.hpp"

#include <cstddef>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
std::string readFixture(const char* name) {
    std::ifstream stream(std::filesystem::path{AVEMOTION_FIXTURE_DIR} / name, std::ios::binary);
    return {std::istreambuf_iterator<char>{stream}, {}};
}
avemotion::render::OwnPrepareResult prepare(const std::string& source) {
    return avemotion::render::prepareOwnAsset(std::as_bytes(std::span{source.data(), source.size()}));
}
std::string replaceMixed(std::string source, const std::string& from, const std::string& to) {
    const auto at = source.find(from);
    require(at != std::string::npos, "mixed fixture replacement exists");
    source.replace(at, from.size(), to);
    return source;
}
void mixedBounds(const avemotion::runtime::EvaluatedPath& path,
                 float left, float top, float right, float bottom) {
    const auto& bounds = path.controlBounds;
    require(bounds.valid && std::abs(bounds.left - left) < 0.0001F
            && std::abs(bounds.top - top) < 0.0001F
            && std::abs(bounds.right - right) < 0.0001F
            && std::abs(bounds.bottom - bottom) < 0.0001F, "literal mixed local geometry bounds");
}
void mixedGeometryUsesCanonicalPlayback() {
    using namespace avemotion;
    auto source = readFixture("own/mixed_geometry.json");
    auto prepared = prepare(source);
    if (!prepared) throw std::runtime_error("mixed preparation failed: " + prepared.error.path
                                            + " " + prepared.error.message);
    const auto fit = render::fitOwnTarget(*prepared.asset, 100, 100);
    require(fit && fit->width == 100 && fit->height == 75, "mixed uses vector aspect fitting");
    auto first = render::createOwnPlayback(prepared.asset);
    auto second = render::createOwnPlayback(prepared.asset);
    require(bool(first) && bool(second), "mixed independent playback creation");
    const auto now = runtime::MotionTime::fromSeconds(0);
    auto retained = first.playback->evaluateAt(now, 128, 96);
    require(bool(retained) && retained.scene->drawItems.size() == 3, "three mixed draws");
    const auto& draws = retained.scene->drawItems;
    const auto& asset = *retained.scene->assetModel;
    const model::SourceNodeKind kinds[]{model::SourceNodeKind::Shape,
        model::SourceNodeKind::Rectangle, model::SourceNodeKind::Ellipse};
    for (unsigned i = 0; i < 3; ++i) {
        const auto& draw = draws[i];
        require(draw.drawOrder == i && asset.sourceNode(draw.sourcePathNode)->kind == kinds[i]
                && asset.sourceNode(draw.sourcePaintNode)->kind == model::SourceNodeKind::Fill,
                "reverse authored geometry and preserved source roles");
        const auto& resource = asset.geometries[draw.modelGeometry.index()];
        require(resource.resourceClass == model::ResourceClass::AssetStatic && resource.staticValue
                && !resource.staticValue->path.verbs.empty() && draw.localGeometryStaticCandidate,
                "static local geometry stays asset owned under transform/opacity animation");
        if (i) require(draw.sourcePathNode != draws[i - 1].sourcePathNode
                       && draw.sourceGeometryId != draws[i - 1].sourceGeometryId,
                       "authored geometries have distinct identities");
    }
    mixedBounds(draws[0].localPath, 88, 12, 112, 44);
    mixedBounds(draws[1].localPath, 48, 22, 80, 42);
    mixedBounds(draws[2].localPath, 14.25F, 22, 34.75F, 34.5F);
    require(draws[0].paint.solid.b == 255 && draws[1].paint.solid.g == 255
            && draws[2].paint.solid.r == 255 && draws[2].paint.solid.a == 127,
            "mixed paint order and transparency");
    const auto retainedHash = retained.scene->fingerprints.scene;
    second.playback->setControlledProgress(0.5);
    const auto midpoint = second.playback->evaluateAt(now, 128, 96);
    require(bool(midpoint) && midpoint.scene->frameIndex == 30
            && midpoint.scene->instanceId != retained.scene->instanceId
            && midpoint.scene->drawItems[2].paint.solid.a == 95
            && midpoint.scene->drawItems[2].localToViewport.dx == 4
            && midpoint.scene->drawItems[2].localToViewport.dy == 2
            && first.playback->playbackSnapshot(now).frameIndex == 0,
            "independent mixed controls and literal transform/opacity midpoint");
    first.playback->setControlledProgress(1);
    require(bool(first.playback->evaluateAt(now, 256, 192)), "mixed can evaluate another frame and viewport");
    first.playback.reset(); second.playback.reset(); prepared.asset.reset(); source.clear();
    require(retained.scene->fingerprints.scene == retainedHash
            && retained.scene->assetModel->geometries[2].staticValue.has_value(),
            "retained mixed scene survives evaluation and owner retirement");
    mixedBounds(retained.scene->drawItems[2].localPath, 14.25F, 22, 34.75F, 34.5F);
    std::cout << "PASS mixed canonical public playback: order, bounds, identity, resources and ownership\n";
}
void animatedMixedGeometrySeeks() {
    using namespace avemotion;
    auto source = readFixture("own/mixed_geometry.json");
    source = replaceMixed(source, R"("p":{"k":[24.5,28.25]})",
        R"("p":{"a":1,"k":[{"t":0,"s":[24.5,28.25],"o":{"x":0,"y":0},"i":{"x":1,"y":1}},{"t":60,"s":[44.5,38.25]}]})");
    source = replaceMixed(source, R"("s":{"k":[20.5,12.5]})",
        R"("s":{"a":1,"k":[{"t":0,"s":[20.5,12.5],"o":{"x":[0.25,0.25],"y":[0,0]},"i":{"x":[0.75,0.75],"y":[1,1]}},{"t":60,"s":[40.5,22.5]}]})");
    source = replaceMixed(source, R"("s":{"k":[32,20]})",
        R"("s":{"a":1,"k":[{"t":0,"s":[32,20],"h":1},{"t":30,"s":[16,12],"h":1},{"t":60,"s":[24,16]}]})");
    source = replaceMixed(source, R"("r":{"k":4})",
        R"("r":{"a":1,"k":[{"t":0,"s":[0],"o":{"x":0,"y":0},"i":{"x":1,"y":1}},{"t":60,"s":[8]}]})");
    const auto prepared = prepare(source);
    require(bool(prepared), "animated mixed preparation");
    auto playback = render::createOwnPlayback(prepared.asset);
    auto independent = render::createOwnPlayback(prepared.asset);
    const auto now = runtime::MotionTime::fromSeconds(0);
    std::optional<runtime::EvaluatedScene> retained;
    std::uint64_t retainedHash = 0;
    for (unsigned frame : {0U, 30U, 60U, 29U, 30U, 0U}) {
        playback.playback->setControlledProgress(static_cast<double>(frame) / 60);
        const auto evaluated = playback.playback->evaluateAt(now, 128, 96);
        require(bool(evaluated) && evaluated.scene->frameIndex == frame, "out-of-order mixed evaluation");
        const auto& draws = evaluated.scene->drawItems;
        const auto& asset = *evaluated.scene->assetModel;
        require(asset.geometries[0].resourceClass == model::ResourceClass::AssetStatic
                && asset.geometries[1].resourceClass == model::ResourceClass::InstanceEvaluated
                && asset.geometries[2].resourceClass == model::ResourceClass::InstanceEvaluated,
                "animated primitive properties classify only their local geometry");
        if (frame == 0) {
            mixedBounds(draws[2].localPath, 14.25F, 22, 34.75F, 34.5F);
            require(draws[1].localPath.verbs[1] == runtime::PathVerb::LineTo,
                    "zero radius starts with sharp rectangle");
        } else if (frame == 30) {
            mixedBounds(draws[2].localPath, 19.25F, 24.5F, 49.75F, 42);
            require(draws[1].localPath.points.front().x == 72
                    && draws[1].localPath.points.front().y == 30,
                    "literal radius midpoint on counterclockwise rounded rectangle");
        } else if (frame == 60) mixedBounds(draws[2].localPath, 24.25F, 27, 64.75F, 49.5F);
        if (frame < 30) mixedBounds(draws[1].localPath, 48, 22, 80, 42);
        else if (frame < 60) mixedBounds(draws[1].localPath, 56, 26, 72, 38);
        else mixedBounds(draws[1].localPath, 52, 24, 76, 40);
        if (!retained && frame == 30) {
            retained = evaluated.scene; retainedHash = retained->fingerprints.scene;
        }
        require(!retained || retained->fingerprints.scene == retainedHash,
                "animated mixed published scenes are immutable across seeks");
        const auto other = independent.playback->evaluateAt(now, 128, 96);
        require(bool(other) && other.scene->frameIndex == 0, "animated mixed instance remains at its own frame");
        mixedBounds(other.scene->drawItems[2].localPath, 14.25F, 22, 34.75F, 34.5F);
    }
    playback.playback.reset(); independent.playback.reset();
    mixedBounds(retained->drawItems[2].localPath, 19.25F, 24.5F, 49.75F, 42);
    mixedBounds(retained->drawItems[1].localPath, 56, 26, 72, 38);
    std::cout << "PASS animated mixed public playback: linear/cubic p/s/r, holds and repeated seeks\n";
}
void invalidMixedGeometryIsContextual() {
    using namespace avemotion;
    const auto fixture = readFixture("own/mixed_geometry.json");
    const auto rejected = [&](const std::string& source, const std::string& path,
                              render::OwnErrorCode code = render::OwnErrorCode::UnsupportedAsset) {
        const auto result = prepare(source);
        require(!result && !result.asset && result.error.path == path && result.error.code == code
                && !result.error.message.empty(), "mixed public rejection is atomic and contextual");
    };
    rejected(replaceMixed(fixture, R"("d":1)", R"("d":2)"), "/layers/0/shapes/0/it/0/d");
    rejected(replaceMixed(fixture, "[20.5,12.5]", "[0,12.5]"), "/layers/0/shapes/0/it/0/s/k/0");
    rejected(replaceMixed(fixture, R"("r":{"k":4})", R"("r":{"k":-1})"), "/layers/0/shapes/1/it/0/r/k");
    rejected(replaceMixed(fixture, R"("p":{"k":[24.5,28.25]})",
        R"("p":{"a":1,"k":[{"t":0,"s":[24.5,28.25],"h":1},{"t":60}]})"),
        "/layers/0/shapes/0/it/0/p/k/1/s");
    rejected(replaceMixed(fixture, R"("ty":"fl")", R"("ty":"st")"), "/layers/0/shapes/0/it");
    rejected(replaceMixed(replaceMixed(fixture, "[24.5,28.25]", "[32768,32768]"), "[20.5,12.5]", "[0.0001,1]"),
        "/layers/0/shapes/0/it/0", render::OwnErrorCode::PreparationFailed);
    auto fractional = prepare(replaceMixed(fixture, "[20.5,12.5]", "[0.5,0.25]"));
    require(bool(fractional), "positive subunit mixed size publicly prepares");
    auto playback = render::createOwnPlayback(fractional.asset);
    const auto frame = playback.playback->evaluateAt(runtime::MotionTime::fromSeconds(0), 128, 96);
    require(bool(frame), "positive subunit mixed size evaluates");
    mixedBounds(frame.scene->drawItems[2].localPath, 24.25F, 28.125F, 24.75F, 28.375F);
    std::cout << "PASS mixed public errors: contextual admission/collapse and positive subunit geometry\n";
}
void individualMixedPropertiesClassifyGeometry() {
    using namespace avemotion;
    // Each property alone must classify and sample its geometry; combined
    // animated properties would conceal an omitted dependency in lowering.
    struct Sample { const char* from; const char* to; unsigned draw; float left, top, right, bottom, firstY; };
    const Sample samples[]{
        {R"("p":{"k":[24.5,28.25]})", R"("p":{"a":1,"k":[{"t":0,"s":[24.5,28.25],"h":1},{"t":30,"s":[34.5,33.25]}]})", 2, 24.25F, 27, 44.75F, 39.5F, 27},
        {R"("s":{"k":[20.5,12.5]})", R"("s":{"a":1,"k":[{"t":0,"s":[20.5,12.5],"h":1},{"t":30,"s":[30.5,17.5]}]})", 2, 9.25F, 19.5F, 39.75F, 37, 19.5F},
        {R"("p":{"k":[64,32]})", R"("p":{"a":1,"k":[{"t":0,"s":[64,32],"h":1},{"t":30,"s":[72,36]}]})", 1, 56, 26, 88, 46, 30},
        {R"("s":{"k":[32,20]})", R"("s":{"a":1,"k":[{"t":0,"s":[32,20],"h":1},{"t":30,"s":[40,24]}]})", 1, 44, 20, 84, 44, 24},
        {R"("r":{"k":4})", R"("r":{"a":1,"k":[{"t":0,"s":[4],"h":1},{"t":30,"s":[6]}]})", 1, 48, 22, 80, 42, 28}
    };
    const auto fixture = readFixture("own/mixed_geometry.json");
    for (const auto& sample : samples) {
        const auto prepared = prepare(replaceMixed(fixture, sample.from, sample.to));
        require(bool(prepared), "single animated primitive property prepares");
        auto playback = render::createOwnPlayback(prepared.asset);
        playback.playback->setControlledProgress(0.5);
        const auto frame = playback.playback->evaluateAt(runtime::MotionTime::fromSeconds(0), 128, 96);
        require(bool(frame), "single animated primitive property evaluates");
        for (unsigned i = 0; i < 3; ++i)
            require(frame.scene->assetModel->geometries[i].resourceClass ==
                    (i == sample.draw ? model::ResourceClass::InstanceEvaluated : model::ResourceClass::AssetStatic),
                    "each p/s/r property independently classifies only its geometry");
        const auto& path = frame.scene->drawItems[sample.draw].localPath;
        mixedBounds(path, sample.left, sample.top, sample.right, sample.bottom);
        require(path.points.front().y == sample.firstY, "single property literal endpoint path");
    }
    std::cout << "PASS mixed independent geometry dependencies: ellipse p/s and rectangle p/s/r\n";
}
void durationAdmissionIsFinite(const char* fixture, const std::string& rateMarker) {
    using namespace avemotion;
    auto source = readFixture(fixture);
    const auto offset = source.find(rateMarker);
    require(offset != std::string::npos, "fixture frame rate found");
    source.replace(offset, rateMarker.size(), "\"fr\":1e-37");
    const auto overflowing = prepare(source);
    source.clear();
    source.shrink_to_fit();
    require(!overflowing.asset && overflowing.error.code == render::OwnErrorCode::UnsupportedAsset
            && overflowing.error.path == "/fr" && !overflowing.error.message.empty(),
            "positive frame rate with overflowing float duration is rejected");

    source = readFixture(fixture);
    source.replace(source.find(rateMarker), rateMarker.size(), "\"fr\":1e-35");
    const auto finite = prepare(source);
    require(bool(finite), "nearby finite-duration frame rate is accepted");
    auto created = render::createOwnPlayback(finite.asset);
    require(bool(created) && std::isfinite(created.playback->durationSeconds())
            && created.playback->durationSeconds() > 0,
            "accepted frame rate has finite positive duration");
    const auto start = runtime::MotionTime::fromSeconds(0);
    created.playback->play(start);
    require(bool(created.playback->evaluateAt(start, 128, 128)),
            "accepted frame rate evaluates its initial frame");
}
void terminalCompletionRequiresExplicitRetry(
    const std::shared_ptr<const avemotion::render::OwnAsset>& asset) {
    using namespace avemotion;
    const auto start = runtime::MotionTime::fromSeconds(0);
    const auto terminal = runtime::MotionTime::fromSeconds(2);
    auto created = render::createOwnPlayback(asset);
    require(bool(created), "public terminal playback creates");
    player::Player player;
    const auto added = render::addOwnPlayback(player, created.playback, start);
    require(bool(added) && player.setLoopMode(added.handle, runtime::PlaybackLoopMode::Once, start)
            && player.play(added.handle, start), "public Once playback starts at zero");
    (void)player.tick(start);
    auto tick = player.tick(terminal);
    require(tick.frames.size() == 1 && tick.frames[0].playback.completed
            && tick.frames[0].playback.frameIndex == 60
            && player::hasReason(tick.frames[0].reasons, player::FrameReason::Completion)
            && !tick.nextDeadline, "terminal tick projects completion and frame 60");
    const auto revision = tick.frames[0].playback.revision;
    require(created.playback->playbackSnapshot(terminal).revision == revision,
            "terminal snapshot does not commit completion");
    const auto failed = created.playback->evaluateAt(terminal, 0, 128);
    require(!failed && failed.error.code == render::OwnErrorCode::InvalidViewport
            && created.playback->playbackSnapshot(terminal).revision == revision,
            "failed terminal evaluation leaves revision unchanged");
    require(player.tick(terminal).frames.empty(), "failed evaluation does not requeue a frame");
    require(player.invalidate(added.handle), "host can request terminal retry");
    tick = player.tick(terminal);
    require(tick.frames.size() == 1
            && player::hasReason(tick.frames[0].reasons, player::FrameReason::ExplicitInvalidation),
            "explicit invalidation returns retry frame");
    const auto success = created.playback->evaluateAt(terminal, 128, 128);
    require(bool(success) && success.scene->frameIndex == 60
            && created.playback->playbackSnapshot(terminal).revision == revision + 1,
            "successful terminal evaluation emits frame 60 and commits once");
    require(bool(created.playback->evaluateAt(terminal, 128, 128))
            && created.playback->playbackSnapshot(terminal).revision == revision + 1,
            "repeat terminal evaluation does not recommit");
}
}

int run() {
    using namespace avemotion;
    mixedGeometryUsesCanonicalPlayback();
    animatedMixedGeometrySeeks();
    invalidMixedGeometryIsContextual();
    individualMixedPropertiesClassifyGeometry();
    std::string source = readFixture("primitive_geometry.json");
    require(!source.empty(), "fixture readable");
    auto prepared = prepare(source);
    require(bool(prepared), "public preparation accepts supported own source");
    require(prepared.asset->metadata().logicalWidth == 280 &&
            prepared.asset->metadata().logicalHeight == 230 &&
            prepared.asset->metadata().totalFrames == 61 &&
            prepared.asset->metadata().frameRate == 60,
            "public metadata retains authored dimensions and timing");
    terminalCompletionRequiresExplicitRetry(prepared.asset);
    durationAdmissionIsFinite("primitive_geometry.json", "\"fr\": 60");
    durationAdmissionIsFinite("own_vector/parent.json", "\"fr\":60");
    auto tinyRateSource = readFixture("own_vector/parent.json");
    const std::string rateMarker = "\"fr\":60";
    const auto rateOffset = tinyRateSource.find(rateMarker);
    require(rateOffset != std::string::npos, "supported vector fixture frame rate found");
    tinyRateSource.replace(rateOffset, rateMarker.size(), "\"fr\":1e-307");
    const auto tinyRate = prepare(tinyRateSource);
    require(!tinyRate && tinyRate.error.code == render::OwnErrorCode::UnsupportedAsset
            && tinyRate.error.path == "/fr",
            "own vector admission rejects rate below finite float representation");
    source.clear();
    source.shrink_to_fit();
    auto created = render::createOwnPlayback(prepared.asset);
    require(bool(created), "public playback creates from prepared asset");
    auto now = runtime::MotionTime::fromSeconds(0);
    created.playback->play(now);
    require(created.playback->playbackSnapshot(now).status == runtime::PlaybackStatus::Playing,
            "public control starts playback");
    auto evaluated = created.playback->evaluateAt(now, 128, 128);
    require(bool(evaluated) && evaluated.scene->instanceId != 0,
            "public playback evaluates supported frame with own identity");
    const auto identity = evaluated.scene->instanceId;
    const auto invalidViewport = created.playback->evaluateAt(now, 0, 128);
    require(!invalidViewport && invalidViewport.error.code == render::OwnErrorCode::InvalidViewport
            && !invalidViewport.error.message.empty(),
            "invalid viewport has owned structured evaluation error");
    require(created.playback->evaluateAt(now, 128, 128).scene->instanceId == identity,
            "failed evaluation preserves own identity and allows retry");
    auto second = render::createOwnPlayback(prepared.asset);
    require(bool(second), "shared immutable asset creates second playback");
    second.playback->setControlledProgress(0.5);
    require(second.playback->playbackSnapshot(now).frameIndex == 30 &&
            created.playback->playbackSnapshot(now).frameIndex == 0,
            "two playback controls remain independent");
    const auto legacyMidpoint = second.playback->evaluateAt(now, 128, 128);
    require(legacyMidpoint.scene->instanceId != identity,
            "distinct playbacks keep distinct own scene identities");
    mixedBounds(legacyMidpoint.scene->drawItems[0].localPath, 65, 168, 120, 207);
    player::Player player;
    auto added = render::addOwnPlayback(player, created.playback, now);
    require(bool(added), "public playback registers in player");
    require(render::ownPlayback(player, added.handle) == created.playback,
            "typed lookup returns public owner");
    auto tick = player.tick(now, player::FrameSelection::AllVisible);
    require(!tick.frames.empty() && tick.frames[0].instance == nullptr
            && !tick.frames[0].instanceHandle.valid(), "own frame has null Runtime identity");
    created.playback.reset();
    require(bool(render::ownPlayback(player, added.handle)), "player retains public owner");
    require(player.removeInstance(added.handle, now), "own registration removes");
    require(!render::ownPlayback(player, added.handle), "removed handle cannot recover playback");
    auto replacement = render::addOwnPlayback(player, second.playback, now);
    require(bool(replacement) && !render::ownPlayback(player, added.handle),
            "stale handle cannot resolve replacement");
    require(render::ownPlayback(player, replacement.handle) == second.playback,
            "replacement resolves its own public owner");
    const auto primitiveFit = render::fitOwnTarget(*prepared.asset, 101, 53);
    require(primitiveFit && primitiveFit->width == 101 && primitiveFit->height == 53,
            "primitive fitting preserves requested box");
    require(!render::fitOwnTarget(*prepared.asset, 0, 53), "zero target rejected");

    require(!prepare("???") && prepare("???").error.code == render::OwnErrorCode::UnknownFormat,
            "unknown bytes have structured format error");
    require(prepare("{broken").error.code == render::OwnErrorCode::InvalidJson,
            "malformed JSON has structured parse error");
    std::string nested = "0";
    for (int i = 0; i < 33; ++i) nested = "[" + nested + "]";
    auto tooDeep = prepare("{\"x\":" + nested + "}");
    require(!tooDeep && tooDeep.error.code == render::OwnErrorCode::ResourceLimit,
            "own JSON depth ceiling enforced");
    const std::string tooLarge(1'048'577, ' ');
    auto huge = prepare("{" + tooLarge + "}");
    require(!huge && huge.error.code == render::OwnErrorCode::ResourceLimit,
            "JSON byte ceiling enforced");
    std::string manyValues = "{\"x\":[";
    for (std::size_t i = 0; i < 65536; ++i) {
        if (i != 0) manyValues += ',';
        manyValues += "null";
    }
    manyValues += "]}";
    require(prepare(manyValues).error.code == render::OwnErrorCode::ResourceLimit,
            "own JSON value ceiling enforced");
    auto unsupported = prepare(R"({"v":"5.7.4","fr":30,"ip":0,"op":2,"w":16,"h":16,"layers":[]})");
    require(!unsupported && unsupported.error.code == render::OwnErrorCode::UnsupportedAsset,
            "unsupported own profile rejects without fallback");
    std::ifstream tgsFile(std::filesystem::path{AVEMOTION_TGS_DIR} / "telegram_sticker_basic.tgs",
                          std::ios::binary);
    const std::string tgsBytes{std::istreambuf_iterator<char>{tgsFile}, {}};
    require(!tgsBytes.empty(), "TGS fixture readable");
    auto tgs = render::prepareOwnAsset(std::as_bytes(std::span{tgsBytes.data(), tgsBytes.size()}));
    require(bool(tgs) && bool(render::createOwnPlayback(tgs.asset)),
            "strict TGS decode reaches same public own engine");
    auto vectorSource = readFixture("own_vector/parent.json");
    const auto dimensions = vectorSource.find("\"w\":128,\"h\":128");
    require(dimensions != std::string::npos, "vector fixture dimensions found");
    vectorSource.replace(dimensions, 15, "\"w\":800,\"h\":420");
    auto vector = prepare(vectorSource);
    require(bool(vector), "vector authored source admitted publicly");
    const auto vectorFit = render::fitOwnTarget(*vector.asset, 100, 53);
    require(vectorFit && vectorFit->width == 100 && vectorFit->height == 52,
            "vector fitting uses bounded integer aspect size");
    require(!render::fitOwnTarget(*vector.asset, 1, 1),
            "vector fitting rejects target collapsing a dimension");
    auto retainedScene = std::move(evaluated.scene);
    player.clear(now);
    second.playback.reset();
    prepared.asset.reset();
    require(retainedScene && retainedScene->assetModel && !retainedScene->drawItems.empty(),
            "evaluated scene retains immutable render ownership after source and playback retire");
    return 0;
}

int main() {
    try { return run(); }
    catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
