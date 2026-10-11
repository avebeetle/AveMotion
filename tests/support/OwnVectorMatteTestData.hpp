#pragma once
#include "OwnVectorGroupTestData.hpp"
#include <array>

namespace avemotion::test {
struct OriginalPartyPair final { int provider, content; unsigned first, last; };
inline constexpr std::array<OriginalPartyPair, 4> originalPartyPairs{{
    {2,3,20,31}, {4,5,20,82}, {6,7,20,82}, {8,9,20,82}}};
// Literal authored identities/intervals, independent of pairing/lowering code.
template<class Prepared>
inline void assertOriginalParty(const Prepared &prepared,
                                const runtime::EvaluatedScene &scene) {
    const auto &authored = *prepared.vectorAuthored;
    vectorRequire(authored.exactJson.size() == 67875 && prepared.model->logicalWidth == 1024 &&
                      prepared.model->logicalHeight == 1024 && prepared.model->frameRate == 60 &&
                      prepared.model->totalFrames == 92 && authored.layers.size() == 11 &&
                      scene.layers.size() == 12, "unchanged Party metadata/inventory");
    const auto sourceId = [&](model::SourceNodeId id) {
        return authored.model->sourceNodes.at(id.index()).authoredLayerId;
    };
    const auto layer = [&](int id) {
        for (std::size_t i=1; i<prepared.program->layers.size(); ++i)
            if (sourceId(prepared.program->layers[i].source) == id) return i;
        throw std::runtime_error("original Party literal authored ID absent");
    };
    std::size_t roles=0;
    for (const auto &pair: originalPartyPairs) {
        const auto p=layer(pair.provider), c=layer(pair.content);
        const auto &provider=scene.layers.at(p), &content=scene.layers.at(c);
        vectorRequire(provider.matteTargetLayer == c && content.matteSourceLayer == p &&
                          provider.matteSourceLayer == runtime::kInvalidSceneIndex &&
                          content.matteTargetLayer == runtime::kInvalidSceneIndex &&
                          provider.matte == runtime::MatteMode::None && content.matte == runtime::MatteMode::Alpha,
                      "Party literal reciprocal pair " + std::to_string(pair.provider));
        vectorRequire(content.visible == (scene.frameIndex>=pair.first && scene.frameIndex<pair.last) &&
                          provider.visible == (scene.frameIndex>=20 && scene.frameIndex<82),
                      "Party literal pair interval");
        const auto &binding=authored.layers.at(static_cast<std::size_t>(pair.provider-1));
        const auto &target=authored.layers.at(static_cast<std::size_t>(pair.content-1));
        vectorRequire(sourceId(binding.matteTarget)==pair.content && sourceId(target.matteSource)==pair.provider &&
                          target.inFrame==pair.first && target.outFrame==pair.last &&
                          (prepared.model->layers.at(provider.modelLayer.index()).dependencyBits & model::StaticDependencyMatte) &&
                          (prepared.model->layers.at(content.modelLayer.index()).dependencyBits & model::StaticDependencyMatte),
                      "Party model literal pairing/dependency");
        roles+=2;
    }
    std::size_t actualRoles=0, differences=0;
    for (const auto &entry:scene.layers) {
        actualRoles += entry.matteSourceLayer!=runtime::kInvalidSceneIndex || entry.matteTargetLayer!=runtime::kInvalidSceneIndex;
        differences += entry.isolatedBottomDifference;
    }
    vectorRequire(actualRoles==roles && differences==1 && scene.layers.at(layer(11)).isolatedBottomDifference &&
                      authored.model->sourceNodes.at(prepared.program->layers.at(layer(11)).source.index()).blendMode==model::SourceBlendMode::Difference,
                  "Party only four pairs and authored isolated bottom Difference");
}
inline std::string matteLayer(int id, const std::string& role = "",
                              const std::string& transform = "", unsigned paints = 1) {
    return vectorReplace(vectorGroupsLayer(vectorGroups(paints), id, transform),
                         "{\"ty\":4", "{\"ty\":4" + role);
}
inline std::string matteFixture() {
    // Authored 2->3; matrix parent is content, not a pairing edge. Null opacity
    // must not propagate. The pair contributes only on [20,31).
    const auto null = vectorReplace(vectorLayer(R"("p":{"k":[5,7]},"o":{"k":0})"),
                                     "\"ind\":1", "\"ind\":1");
    auto provider = matteLayer(2, ",\"td\":1,\"parent\":3", R"("p":{"k":[2,4]},"o":{"k":50})");
    auto content = matteLayer(3, ",\"tt\":1,\"parent\":1", R"("p":{"k":[10,20]},"o":{"k":50})");
    provider = vectorReplace(vectorReplace(provider,"\"ip\":0","\"ip\":20"),"\"op\":180","\"op\":31");
    content = vectorReplace(vectorReplace(content,"\"ip\":0","\"ip\":20"),"\"op\":180","\"op\":31");
    return vectorRoot(null + "," + provider + "," + content);
}
} // namespace avemotion::test
