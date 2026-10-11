#pragma once
#include "OwnVectorGroupTestData.hpp"

namespace avemotion::test {
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
