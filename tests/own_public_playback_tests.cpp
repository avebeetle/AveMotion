#include "avemotion/render/OwnPlayback.hpp"

#include <cstddef>
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
}

int run() {
    using namespace avemotion;
    std::string source = readFixture("primitive_geometry.json");
    require(!source.empty(), "fixture readable");
    auto prepared = prepare(source);
    require(bool(prepared), "public preparation accepts supported own source");
    require(prepared.asset->metadata().logicalWidth == 280 &&
            prepared.asset->metadata().logicalHeight == 230 &&
            prepared.asset->metadata().totalFrames == 61 &&
            prepared.asset->metadata().frameRate == 60,
            "public metadata retains authored dimensions and timing");
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
    require(second.playback->evaluateAt(now, 128, 128).scene->instanceId != identity,
            "distinct playbacks keep distinct own scene identities");
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
